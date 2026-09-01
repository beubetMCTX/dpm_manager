#include "species_material_dialog.h"
#include "qUI_components.h"
#include "runtime_debug.h"
#include "ui_species_material_dialog.h"

#include <QAbstractItemView>
#include <QCloseEvent>
#include <QHeaderView>
#include <QPushButton>
#include <QStyledItemDelegate>
#include <QTableWidgetItem>

namespace
{
class DensityItemDelegate final : public QStyledItemDelegate
{
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override
    {
        if (index.column() != 1)
        {
            return QStyledItemDelegate::createEditor(parent, option, index);
        }

        auto *line_edit = new QUI_LineEdit(parent);
        line_edit->set_double_mode();
        line_edit->setPlaceholderText("Supports expressions like 5+6 or sin(30)");
        return line_edit;
    }
};

QString normalized_material_name(const QTableWidgetItem *item)
{
    return item != nullptr ? item->text().trimmed() : QString();
}

double density_value_from_item(const QTableWidgetItem *item)
{
    if (item == nullptr)
    {
        return 0.0;
    }

    bool ok = false;
    const double value = item->text().trimmed().toDouble(&ok);
    return ok ? value : 0.0;
}
}

SpeciesMaterialDialog::SpeciesMaterialDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SpeciesMaterialDialog)
{
    runtime_debug::trace("SpeciesMaterialDialog constructor begin");
    ui->setupUi(this);
    // Let the MainWindow own this reusable auxiliary dialog until shutdown.
    setAttribute(Qt::WA_DeleteOnClose, false);

    ui->materialsTable->setEditTriggers(
        QAbstractItemView::DoubleClicked |
        QAbstractItemView::EditKeyPressed |
        QAbstractItemView::SelectedClicked);
    ui->materialsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->materialsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->materialsTable->setAlternatingRowColors(true);
    ui->materialsTable->verticalHeader()->setVisible(false);
    ui->materialsTable->horizontalHeader()->setStretchLastSection(false);
    ui->materialsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->materialsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->materialsTable->setItemDelegateForColumn(1, new DensityItemDelegate(ui->materialsTable));
    ui->materialsTable->setStyleSheet(qui_table_widget_style_sheet());

    connect(ui->addRowButton, &QPushButton::clicked, this, [this]()
    {
        add_empty_row();
    });
    connect(ui->removeRowButton, &QPushButton::clicked, this, [this]()
    {
        remove_current_row();
    });
    connect(ui->materialsTable, &QTableWidget::itemChanged, this,
            [this](QTableWidgetItem *item)
    {
        handle_item_changed(item);
    });
    connect(ui->materialFilterEdit, &QLineEdit::textChanged, this,
            [this](const QString &text)
    {
        apply_material_filter(text);
    });

    add_empty_row();
    adjustSize();
    setFixedSize(sizeHint());
    runtime_debug::trace("SpeciesMaterialDialog constructor end");
}

SpeciesMaterialDialog::~SpeciesMaterialDialog()
{
    runtime_debug::trace("SpeciesMaterialDialog destructor begin");
    delete ui;
    runtime_debug::trace("SpeciesMaterialDialog destructor end");
}

void SpeciesMaterialDialog::closeEvent(QCloseEvent *event)
{
    runtime_debug::trace("SpeciesMaterialDialog closeEvent begin");
    QDialog::closeEvent(event);
    runtime_debug::trace("SpeciesMaterialDialog closeEvent end");
}

void SpeciesMaterialDialog::set_material_entries(const QList<MaterialConfigEntry> &entries)
{
    if (ui == nullptr || ui->materialsTable == nullptr)
    {
        return;
    }

    m_syncing_table = true;
    ui->materialsTable->setRowCount(0);

    for (const MaterialConfigEntry &entry : entries)
    {
        const QString trimmed_name = entry.name.trimmed();
        if (trimmed_name.isEmpty())
        {
            continue;
        }

        const int row = ui->materialsTable->rowCount();
        ui->materialsTable->insertRow(row);
        ensure_row_items_exist(row);
        ui->materialsTable->item(row, 0)->setText(trimmed_name);
        ui->materialsTable->item(row, 1)->setText(QString::number(entry.density, 'g', 15));
    }

    if (ui->materialsTable->rowCount() == 0)
    {
        ui->materialsTable->insertRow(0);
        ensure_row_items_exist(0);
    }

    m_syncing_table = false;
    apply_material_filter(ui->materialFilterEdit->text());
    ui->materialsTable->setCurrentCell(0, 0);
    update_summary_label();
}

QList<MaterialConfigEntry> SpeciesMaterialDialog::material_entries() const
{
    QList<MaterialConfigEntry> entries;
    if (ui == nullptr || ui->materialsTable == nullptr)
    {
        return entries;
    }

    QStringList names;
    for (int row = 0; row < ui->materialsTable->rowCount(); ++row)
    {
        const QString name = normalized_material_name(ui->materialsTable->item(row, 0));
        if (name.isEmpty() || names.contains(name))
        {
            continue;
        }

        MaterialConfigEntry entry;
        entry.name = name;
        entry.density = density_value_from_item(ui->materialsTable->item(row, 1));
        entries.append(entry);
        names.append(name);
    }

    return entries;
}

void SpeciesMaterialDialog::add_empty_row()
{
    if (ui == nullptr || ui->materialsTable == nullptr)
    {
        return;
    }

    const int row = ui->materialsTable->rowCount();
    m_syncing_table = true;
    ui->materialsTable->insertRow(row);
    ensure_row_items_exist(row);
    m_syncing_table = false;

    ui->materialsTable->setCurrentCell(row, 0);
    ui->materialsTable->editItem(ui->materialsTable->item(row, 0));
    update_summary_label();
    emit_materials_changed();
}

void SpeciesMaterialDialog::remove_current_row()
{
    if (ui == nullptr || ui->materialsTable == nullptr)
    {
        return;
    }

    const int row = ui->materialsTable->currentRow();
    if (row < 0)
    {
        return;
    }

    m_syncing_table = true;
    ui->materialsTable->removeRow(row);
    m_syncing_table = false;

    if (ui->materialsTable->rowCount() == 0)
    {
        add_empty_row();
        return;
    }

    const int next_row = qMin(row, ui->materialsTable->rowCount() - 1);
    ui->materialsTable->setCurrentCell(next_row, 0);
    update_summary_label();
    emit_materials_changed();
}

void SpeciesMaterialDialog::ensure_row_items_exist(int row)
{
    if (ui == nullptr || ui->materialsTable == nullptr || row < 0)
    {
        return;
    }

    if (ui->materialsTable->item(row, 0) == nullptr)
    {
        auto *name_item = new QTableWidgetItem();
        name_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        ui->materialsTable->setItem(row, 0, name_item);
    }

    if (ui->materialsTable->item(row, 1) == nullptr)
    {
        auto *density_item = new QTableWidgetItem();
        density_item->setTextAlignment(Qt::AlignVCenter | Qt::AlignRight);
        ui->materialsTable->setItem(row, 1, density_item);
    }
}

void SpeciesMaterialDialog::update_summary_label() const
{
    if (ui == nullptr || ui->summaryLabel == nullptr || ui->materialsTable == nullptr)
    {
        return;
    }

    ui->summaryLabel->setText(
        QString("Materials: %1").arg(material_entries().size()));
}

void SpeciesMaterialDialog::handle_item_changed(QTableWidgetItem *item)
{
    if (m_syncing_table || item == nullptr || ui == nullptr || ui->materialsTable == nullptr)
    {
        return;
    }

    ensure_row_items_exist(item->row());

    if (item->column() == 0)
    {
        item->setText(item->text().trimmed());
    }
    else if (item->column() == 1)
    {
        item->setText(item->text().trimmed());
    }

    update_summary_label();
    emit_materials_changed();
}

void SpeciesMaterialDialog::emit_materials_changed()
{
    if (m_syncing_table)
    {
        return;
    }

    emit materials_changed();
}

void SpeciesMaterialDialog::apply_material_filter(const QString &filter_text)
{
    if (ui == nullptr || ui->materialsTable == nullptr)
    {
        return;
    }

    const QString needle = filter_text.trimmed().toCaseFolded();
    for (int row = 0; row < ui->materialsTable->rowCount(); ++row)
    {
        const QTableWidgetItem *item = ui->materialsTable->item(row, 0);
        const bool visible = needle.isEmpty() ||
            (item != nullptr && item->text().toCaseFolded().contains(needle));
        ui->materialsTable->setRowHidden(row, !visible);
    }
}
