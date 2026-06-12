#include "species_color_dialog.h"
#include "app_config.h"
#include "qUI_components.h"
#include "runtime_debug.h"
#include "ui_species_color_dialog.h"

#include <QAbstractItemView>
#include <QColor>
#include <QColorDialog>
#include <QCloseEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLayout>
#include <QMessageBox>
#include <QPainter>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>

namespace
{
class SpeciesColorItemDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        if ((option.state & QStyle::State_Selected) == 0)
        {
            return;
        }

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->fillRect(option.rect.adjusted(1, 1, -1, -1), QColor(255, 255, 255, 36));
        painter->setPen(QPen(QColor(255, 196, 64), 2));
        painter->drawRect(option.rect.adjusted(1, 1, -2, -2));
        painter->restore();
    }
};

QColor placeholder_color_for_species(const QString &species_name)
{
    const uint hash_value = qHash(species_name);
    const int hue = static_cast<int>(hash_value % 360U);
    const int saturation = 110 + static_cast<int>((hash_value / 360U) % 90U);
    const int value = 180 + static_cast<int>((hash_value / 32400U) % 60U);
    return QColor::fromHsv(hue, saturation, value);
}
}

SpeciesColorDialog::SpeciesColorDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SpeciesColorDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose, true);

    ui->speciesTable->setEditTriggers(
        QAbstractItemView::DoubleClicked |
        QAbstractItemView::EditKeyPressed |
        QAbstractItemView::SelectedClicked);
    ui->speciesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->speciesTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->speciesTable->setAlternatingRowColors(true);
    ui->speciesTable->verticalHeader()->setVisible(false);
    ui->speciesTable->horizontalHeader()->setStretchLastSection(false);
    ui->speciesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->speciesTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->speciesTable->setItemDelegate(new SpeciesColorItemDelegate(ui->speciesTable));
    ui->speciesTable->setStyleSheet(qui_table_widget_style_sheet());

    m_color_dialog = new QColorDialog(this);
    m_color_dialog->setWindowFlags(Qt::Widget);
    m_color_dialog->setOption(QColorDialog::DontUseNativeDialog, true);
    m_color_dialog->setOption(QColorDialog::NoButtons, true);
    m_color_dialog->setOption(QColorDialog::ShowAlphaChannel, false);
    m_color_dialog->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    ui->colorDialogHostLayout->addWidget(m_color_dialog);

    if (layout() != nullptr)
    {
        layout()->activate();
    }
    adjustSize();
    setFixedSize(sizeHint());

    update_editor_panel_idle_state();

    connect(ui->speciesTable, &QTableWidget::currentCellChanged, this,
            [this](int, int, int, int)
    {
        sync_color_picker_to_current_row();
    });
    connect(ui->speciesTable, &QTableWidget::itemChanged, this,
            [this](QTableWidgetItem *item)
    {
        if (item == nullptr || item->column() != 1 || m_syncing_from_picker || m_syncing_from_table)
        {
            return;
        }

        const QString species_name = ui->speciesTable->item(item->row(), 0) != nullptr
            ? ui->speciesTable->item(item->row(), 0)->text()
            : QString();
        if (species_name.isEmpty())
        {
            return;
        }

        const QColor parsed_color(item->text().trimmed());
        if (!parsed_color.isValid())
        {
            m_syncing_from_table = true;
            const QColor fallback_color = color_for_species(species_name);
            item->setText(fallback_color.name(QColor::HexRgb).toUpper());
            update_color_item_visuals(item, fallback_color);
            m_syncing_from_table = false;
            return;
        }

        if (!try_set_color_for_species(species_name, parsed_color, true))
        {
            m_syncing_from_table = true;
            const QColor fallback_color = color_for_species(species_name);
            item->setText(fallback_color.name(QColor::HexRgb).toUpper());
            update_color_item_visuals(item, fallback_color);
            m_syncing_from_table = false;
        }
    });
    connect(m_color_dialog, &QColorDialog::currentColorChanged, this,
            [this](const QColor &color)
    {
        if (m_syncing_from_table || !color.isValid() || m_editing_species_name.isEmpty())
        {
            return;
        }

        m_pending_color = color;
        ui->editorLabel->setText(
            QString("Editing color for %1. Preview: %2.")
                .arg(m_editing_species_name, color.name(QColor::HexRgb).toUpper()));
    });
    connect(ui->applyButton, &QPushButton::clicked, this, [this]()
    {
        if (!m_editing_species_name.isEmpty() && m_pending_color.isValid())
        {
            if (!try_set_color_for_species(m_editing_species_name, m_pending_color, true))
            {
                return;
            }
        }
        m_original_color = m_pending_color;
    if (!m_editing_species_name.isEmpty() && m_pending_color.isValid())
    {
        ui->editorLabel->setText(
            QString("Applied color for %1. You can continue adjusting or select another species.")
                .arg(m_editing_species_name));
    }
    });
    connect(ui->cancelButton, &QPushButton::clicked, this, [this]()
    {
        if (m_color_dialog != nullptr && m_original_color.isValid())
        {
            QSignalBlocker blocker(m_color_dialog);
            m_color_dialog->setCurrentColor(m_original_color);
        }
        m_pending_color = m_original_color;
    if (!m_editing_species_name.isEmpty() && m_original_color.isValid())
    {
        ui->editorLabel->setText(
            QString("Restored color for %1. You can continue adjusting or select another species.")
                .arg(m_editing_species_name));
    }
    });
    connect(ui->saveButton, &QPushButton::clicked, this, [this]()
    {
        save_colors_for_current_chemkin();
    });

    rebuild_table();
}

SpeciesColorDialog::~SpeciesColorDialog()
{
    runtime_debug::trace("SpeciesColorDialog destructor begin");
    delete ui;
    runtime_debug::trace("SpeciesColorDialog destructor end");
}

void SpeciesColorDialog::set_species_names(const QStringList &species_names)
{
    set_chemkin_context(m_chemkin_file_path, species_names);
}

void SpeciesColorDialog::set_chemkin_context(const QString &chemkin_file_path,
                                             const QStringList &species_names)
{
    const bool same_context =
        (m_chemkin_file_path == chemkin_file_path && m_species_names == species_names);

    m_chemkin_file_path = chemkin_file_path;
    m_species_names = species_names;

    if (same_context)
    {
        rebuild_table();
        return;
    }

    m_species_colors.clear();
    load_colors_for_current_chemkin();
    rebuild_table();
}

void SpeciesColorDialog::closeEvent(QCloseEvent *event)
{
    runtime_debug::trace("SpeciesColorDialog closeEvent begin");
    QDialog::closeEvent(event);
    runtime_debug::trace("SpeciesColorDialog closeEvent end");
}

void SpeciesColorDialog::clear_current_selection()
{
    if (ui == nullptr || ui->speciesTable == nullptr)
    {
        return;
    }

    if (QItemSelectionModel *selection_model = ui->speciesTable->selectionModel())
    {
        QSignalBlocker blocker(selection_model);
        selection_model->clear();
        selection_model->setCurrentIndex(QModelIndex(), QItemSelectionModel::Clear);
    }

    ui->speciesTable->setCurrentItem(nullptr);
    sync_color_picker_to_current_row();
}

void SpeciesColorDialog::update_editor_panel_idle_state()
{
    m_editing_species_name.clear();
    m_original_color = QColor();
    m_pending_color = QColor();

    if (ui != nullptr && ui->editorLabel != nullptr)
    {
        ui->editorLabel->setText("Select a species row to edit its color. The palette will stay visible here.");
    }
    if (m_color_dialog != nullptr)
    {
        m_color_dialog->setEnabled(false);
    }
    if (ui != nullptr && ui->applyButton != nullptr)
    {
        ui->applyButton->setEnabled(false);
    }
    if (ui != nullptr && ui->cancelButton != nullptr)
    {
        ui->cancelButton->setEnabled(false);
    }
    if (ui != nullptr && ui->saveButton != nullptr)
    {
        ui->saveButton->setEnabled(!m_species_names.isEmpty() && !m_chemkin_file_path.trimmed().isEmpty());
    }
}

bool SpeciesColorDialog::can_assign_color(const QString &species_name,
                                          const QColor &color,
                                          QString *conflict_species_name) const
{
    if (conflict_species_name != nullptr)
    {
        conflict_species_name->clear();
    }

    if (species_name.isEmpty() || !color.isValid())
    {
        return false;
    }

    const QString normalized_color = color.name(QColor::HexRgb).toUpper();
    for (const QString &other_species_name : m_species_names)
    {
        if (other_species_name == species_name)
        {
            continue;
        }

        if (color_for_species(other_species_name).name(QColor::HexRgb).toUpper() == normalized_color)
        {
            if (conflict_species_name != nullptr)
            {
                *conflict_species_name = other_species_name;
            }
            return false;
        }
    }

    return true;
}

bool SpeciesColorDialog::try_set_color_for_species(const QString &species_name,
                                                   const QColor &color,
                                                   bool show_warning_on_conflict)
{
    if (species_name.isEmpty() || !color.isValid())
    {
        return false;
    }

    QString conflict_species_name;
    if (!can_assign_color(species_name, color, &conflict_species_name))
    {
        const QColor fallback_color = color_for_species(species_name);
        const QString message = QString(
            "Color %1 is already assigned to species %2. %3 was rolled back to %4.")
                .arg(color.name(QColor::HexRgb).toUpper(),
                     conflict_species_name,
                     species_name,
                     fallback_color.name(QColor::HexRgb).toUpper());

        qWarning() << message;
        emit warning_message_requested(message);

        if (show_warning_on_conflict)
        {
            QMessageBox::warning(this, "Species Color Conflict", message);
        }

        if (current_species_name() == species_name && m_color_dialog != nullptr)
        {
            QSignalBlocker blocker(m_color_dialog);
            m_syncing_from_picker = true;
            m_color_dialog->setCurrentColor(fallback_color);
            m_syncing_from_picker = false;
        }

        if (m_editing_species_name == species_name)
        {
            m_pending_color = fallback_color;
            ui->editorLabel->setText(message);
        }

        return false;
    }

    set_color_for_species(species_name, color);
    return true;
}

void SpeciesColorDialog::load_colors_for_current_chemkin()
{
    if (m_chemkin_file_path.trimmed().isEmpty() || m_species_names.isEmpty())
    {
        return;
    }

    QString error_message;
    QHash<QString, QColor> loaded_colors;
    if (!load_species_color_config(m_chemkin_file_path, m_species_names, &loaded_colors, &error_message))
    {
        if (!error_message.trimmed().isEmpty())
        {
            qWarning() << error_message;
            emit warning_message_requested(error_message);
        }
        return;
    }

    m_species_colors = loaded_colors;
}

void SpeciesColorDialog::save_colors_for_current_chemkin()
{
    if (m_chemkin_file_path.trimmed().isEmpty() || m_species_names.isEmpty())
    {
        const QString message = "No Chemkin file context is available yet, so species colors cannot be saved.";
        emit warning_message_requested(message);
        QMessageBox::warning(this, "Save Species Colors", message);
        return;
    }

    QHash<QString, QColor> colors_to_save;
    for (const QString &species_name : m_species_names)
    {
        const QColor color = color_for_species(species_name);
        if (color.isValid())
        {
            colors_to_save.insert(species_name, color);
        }
    }

    QString error_message;
    if (!save_species_color_config(m_chemkin_file_path, m_species_names, colors_to_save, &error_message))
    {
        if (error_message.trimmed().isEmpty())
        {
            error_message = "Failed to save species colors.";
        }
        qWarning() << error_message;
        emit warning_message_requested(error_message);
        QMessageBox::warning(this, "Save Species Colors", error_message);
        return;
    }

    m_species_colors = colors_to_save;
    const QString saved_file_name = QFileInfo(m_chemkin_file_path).fileName();
    ui->editorLabel->setText(
        QString("Saved %1 species colors for %2.")
            .arg(m_species_names.size())
            .arg(saved_file_name));
}

void SpeciesColorDialog::set_color_for_species(const QString &species_name, const QColor &color)
{
    if (species_name.isEmpty() || !color.isValid())
    {
        return;
    }

    m_species_colors.insert(species_name, color);

    for (int row = 0; row < ui->speciesTable->rowCount(); ++row)
    {
        QTableWidgetItem *species_item = ui->speciesTable->item(row, 0);
        QTableWidgetItem *color_item = ui->speciesTable->item(row, 1);
        if (species_item == nullptr || color_item == nullptr || species_item->text() != species_name)
        {
            continue;
        }

        m_syncing_from_table = true;
        color_item->setText(color.name(QColor::HexRgb).toUpper());
        update_color_item_visuals(color_item, color);
        m_syncing_from_table = false;
        break;
    }

    if (current_species_name() == species_name && m_color_dialog != nullptr)
    {
        QSignalBlocker blocker(m_color_dialog);
        m_syncing_from_picker = true;
        m_color_dialog->setCurrentColor(color);
        m_syncing_from_picker = false;
    }

    if (m_editing_species_name == species_name)
    {
        m_original_color = color;
        m_pending_color = color;
    }
}

void SpeciesColorDialog::sync_color_picker_to_current_row()
{
    if (ui == nullptr || ui->editorLabel == nullptr || m_color_dialog == nullptr || ui->editorPanel == nullptr)
    {
        return;
    }

    const QString species_name = current_species_name();
    if (species_name.isEmpty())
    {
        update_editor_panel_idle_state();
        return;
    }

    m_editing_species_name = species_name;
    m_original_color = color_for_species(species_name);
    m_pending_color = m_original_color;
    m_color_dialog->setEnabled(true);
    ui->applyButton->setEnabled(true);
    ui->cancelButton->setEnabled(true);
    ui->saveButton->setEnabled(!m_chemkin_file_path.trimmed().isEmpty());

    ui->editorLabel->setText(
        QString("Editing color for %1. Preview: %2.")
            .arg(species_name, m_original_color.name(QColor::HexRgb).toUpper()));

    QSignalBlocker blocker(m_color_dialog);
    m_syncing_from_picker = true;
    m_color_dialog->setCurrentColor(m_original_color);
    m_syncing_from_picker = false;
}

void SpeciesColorDialog::update_color_item_visuals(QTableWidgetItem *item, const QColor &color) const
{
    if (item == nullptr)
    {
        return;
    }

    item->setToolTip(color.name(QColor::HexRgb).toUpper());
    item->setBackground(color);
    const int brightness = qGray(color.rgb());
    item->setForeground(brightness < 140 ? Qt::white : Qt::black);
}

QColor SpeciesColorDialog::color_for_species(const QString &species_name) const
{
    const auto it = m_species_colors.constFind(species_name);
    if (it != m_species_colors.constEnd())
    {
        return it.value();
    }

    return placeholder_color_for_species(species_name);
}

QString SpeciesColorDialog::current_species_name() const
{
    if (ui == nullptr || ui->speciesTable == nullptr)
    {
        return {};
    }

    const int row = ui->speciesTable->currentRow();
    if (row < 0)
    {
        return {};
    }

    QTableWidgetItem *species_item = ui->speciesTable->item(row, 0);
    return species_item != nullptr ? species_item->text() : QString();
}

void SpeciesColorDialog::rebuild_table()
{
    if (ui == nullptr || ui->summaryLabel == nullptr || ui->speciesTable == nullptr
        || ui->editorLabel == nullptr || m_color_dialog == nullptr)
    {
        return;
    }

    if (m_species_names.isEmpty())
    {
        ui->summaryLabel->setText(
            "No Chemkin species are loaded yet. Import a Chemkin file first, then reopen this window.");
        ui->speciesTable->setRowCount(0);
        update_editor_panel_idle_state();
        return;
    }

    ui->summaryLabel->setText(
        QString("Loaded %1 species.").arg(m_species_names.size()));

    const QString previously_selected_species = current_species_name();
    m_syncing_from_table = true;
    ui->speciesTable->setRowCount(m_species_names.size());
    for (int row = 0; row < m_species_names.size(); ++row)
    {
        const QString &species_name = m_species_names.at(row);
        const QColor display_color = color_for_species(species_name);

        auto *species_item = new QTableWidgetItem(species_name);
        species_item->setToolTip(species_name);
        species_item->setFlags(species_item->flags() & ~Qt::ItemIsEditable);
        ui->speciesTable->setItem(row, 0, species_item);

        auto *color_item = new QTableWidgetItem(display_color.name(QColor::HexRgb).toUpper());
        update_color_item_visuals(color_item, display_color);
        ui->speciesTable->setItem(row, 1, color_item);
    }
    m_syncing_from_table = false;

    ui->speciesTable->resizeColumnToContents(1);

    int row_to_select = m_species_names.isEmpty() ? -1 : 0;
    if (!previously_selected_species.isEmpty())
    {
        for (int row = 0; row < m_species_names.size(); ++row)
        {
            if (m_species_names.at(row) == previously_selected_species)
            {
                row_to_select = row;
                break;
            }
        }
    }

    if (row_to_select >= 0)
    {
        ui->speciesTable->setCurrentCell(row_to_select, 1);
    }
    else
    {
        clear_current_selection();
    }

    sync_color_picker_to_current_row();
}
