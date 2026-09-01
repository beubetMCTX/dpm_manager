#ifndef SPECIES_MATERIAL_DIALOG_H
#define SPECIES_MATERIAL_DIALOG_H

#include "app_config.h"

#include <QDialog>
class QCloseEvent;
class QTableWidgetItem;

namespace Ui {
class SpeciesMaterialDialog;
}

class SpeciesMaterialDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SpeciesMaterialDialog(QWidget *parent = nullptr);
    ~SpeciesMaterialDialog() override;

    void set_material_entries(const QList<MaterialConfigEntry> &entries);
    QList<MaterialConfigEntry> material_entries() const;

signals:
    void materials_changed();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void add_empty_row();
    void remove_current_row();
    void ensure_row_items_exist(int row);
    void update_summary_label() const;
    void handle_item_changed(QTableWidgetItem *item);
    void emit_materials_changed();
    void apply_material_filter(const QString &filter_text);

    Ui::SpeciesMaterialDialog *ui = nullptr;
    bool m_syncing_table = false;
};

#endif // SPECIES_MATERIAL_DIALOG_H
