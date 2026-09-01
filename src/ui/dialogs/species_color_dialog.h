#ifndef SPECIES_COLOR_DIALOG_H
#define SPECIES_COLOR_DIALOG_H

#include <QDialog>
#include <QHash>
#include <QColor>
#include <QStringList>

class QColorDialog;
class QTableWidgetItem;
class QCloseEvent;

namespace Ui {
class SpeciesColorDialog;
}

class SpeciesColorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpeciesColorDialog(QWidget *parent = nullptr);
    ~SpeciesColorDialog() override;

    void set_species_names(const QStringList &species_names);
    void set_chemkin_context(const QString &chemkin_file_path, const QStringList &species_names);
    QHash<QString, QColor> species_colors() const { return m_species_colors; }
    void set_species_colors(const QHash<QString, QColor> &species_colors);

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void warning_message_requested(const QString &message);
    void species_colors_changed();

private:
    bool can_assign_color(const QString &species_name,
                          const QColor &color,
                          QString *conflict_species_name = nullptr) const;
    bool try_set_color_for_species(const QString &species_name,
                                   const QColor &color,
                                   bool show_warning_on_conflict = true);
    void load_colors_for_current_chemkin();
    void save_colors_for_current_chemkin();
    void clear_current_selection();
    void set_color_for_species(const QString &species_name, const QColor &color);
    void sync_color_picker_to_current_row();
    void update_editor_panel_idle_state();
    void update_color_item_visuals(QTableWidgetItem *item, const QColor &color) const;
    QColor color_for_species(const QString &species_name) const;
    QString current_species_name() const;
    void apply_species_filter(const QString &filter_text);
    void rebuild_table();

    QHash<QString, QColor> m_species_colors;
    QStringList m_species_names;
    QString m_chemkin_file_path;
    QString m_editing_species_name;
    QColor m_original_color;
    QColor m_pending_color;
    Ui::SpeciesColorDialog *ui = nullptr;
    QColorDialog *m_color_dialog = nullptr;
    bool m_syncing_from_picker = false;
    bool m_syncing_from_table = false;
};

#endif // SPECIES_COLOR_DIALOG_H
