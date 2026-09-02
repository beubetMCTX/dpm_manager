#include "unit_edit_dialog.h"

#include <QApplication>
#include <QPointer>
#include <QPushButton>
#include <QWidget>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
    {
        qCritical() << message;
        return false;
    }
    return true;
}
}

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    auto *parent = new QWidget();
    Unit unit;
    unit.inj.injector_data.name = "lifetime-test";

    QPointer<unit_edit_dialog> dialog = new unit_edit_dialog(
        &unit, {"O2", "N2"}, {"water"}, parent);
    dialog->show();
    application.processEvents();

    if (!check(dialog != nullptr, "Unit editor should be alive after construction"))
    {
        delete parent;
        return 1;
    }

    if (!check(dialog->findChild<QPushButton*>("applyChangesButton") != nullptr &&
                   dialog->findChild<QPushButton*>("cancelChangesButton") != nullptr,
               "Unit editor should expose explicit apply and cancel actions"))
    {
        delete parent;
        return 1;
    }

    auto *material_editor = dialog->findChild<QUI_ComboBox*>("materialEditor");
    auto *diameter_editor = dialog->findChild<QUI_ComboBox*>("diameterDistributionEditor");
    auto *evaporating_species_editor =
        dialog->findChild<QUI_ComboBox*>("evaporatingSpeciesEditor");
    auto *devolatilizing_species_editor =
        dialog->findChild<QUI_ComboBox*>("devolatilizingSpeciesEditor");
    if (!check(material_editor != nullptr && diameter_editor != nullptr &&
                   evaporating_species_editor != nullptr &&
                   devolatilizing_species_editor != nullptr,
               "Particle-dependent editors should be discoverable"))
    {
        delete parent;
        return 1;
    }

    const QList<QPair<DPM_Type, QList<bool>>> particle_expectations = {
        {Massless, {false, false, false, false}},
        {Inert, {false, false, false, false}},
        {Droplet, {true, true, true, false}},
        {Combusting, {true, false, false, true}},
        {Multicomponent, {true, false, false, false}}
    };
    for (const auto &expectation : particle_expectations)
    {
        unit.inj.injector_data.type = expectation.first;
        dialog->refresh_from_unit_data(&unit);
        application.processEvents();
        const QList<bool> actual = {
            material_editor->isEnabled(),
            diameter_editor->isEnabled(),
            evaporating_species_editor->isEnabled(),
            devolatilizing_species_editor->isEnabled()
        };
        if (!check(actual == expectation.second,
                   "Particle-dependent editor enablement is incorrect"))
        {
            delete parent;
            return 1;
        }
    }

    auto *cone_type_editor = dialog->findChild<QComboBox*>("comboBox_conetype");
    if (!check(cone_type_editor != nullptr,
               "Cone type editor should be available"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = single;
    dialog->refresh_from_unit_data(&unit);
    if (!check(!cone_type_editor->isVisible(),
               "Cone parameter panel should be hidden for non-cone injections"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = cone;
    dialog->refresh_from_unit_data(&unit);
    if (!check(cone_type_editor->isVisible(),
               "Cone parameter panel should be visible for cone injections"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = single;
    unit.inj.injector_data.stochastic = true;
    unit.inj.injector_data.cloud = true;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    auto *stochastic_editor = dialog->findChild<QUI_CheckBox*>("stochasticTrackingEditor");
    auto *cloud_editor = dialog->findChild<QUI_CheckBox*>("cloudTrackingEditor");
    if (!check(stochastic_editor != nullptr && cloud_editor != nullptr &&
                   unit.inj.injector_data.stochastic &&
                   !unit.inj.injector_data.cloud &&
                   stochastic_editor->isEnabled() &&
                   !cloud_editor->isEnabled() &&
                   dialog->findChild<QWidget*>("modelRow_Random_Eddy") != nullptr &&
                   dialog->findChild<QWidget*>("modelRow_Eddy_Attempts") != nullptr &&
                   dialog->findChild<QWidget*>("modelRow_Time_Scale_Constant") != nullptr,
               "Turbulent-dispersion models should normalize and expose stochastic controls"))
    {
        delete parent;
        return 1;
    }
    stochastic_editor->click();
    application.processEvents();
    cloud_editor = dialog->findChild<QUI_CheckBox*>("cloudTrackingEditor");
    if (!check(cloud_editor != nullptr && cloud_editor->isEnabled() &&
                   !unit.inj.injector_data.stochastic &&
                   !unit.inj.injector_data.cloud &&
                   dialog->findChild<QWidget*>("modelRow_Random_Eddy") == nullptr &&
                   dialog->findChild<QWidget*>("modelRow_Cloud_Minimum_Diameter") == nullptr,
               "Disabling stochastic tracking should reveal cloud tracking"))
    {
        delete parent;
        return 1;
    }
    cloud_editor->click();
    application.processEvents();
    stochastic_editor = dialog->findChild<QUI_CheckBox*>("stochasticTrackingEditor");
    if (!check(stochastic_editor != nullptr && !stochastic_editor->isEnabled() &&
                   unit.inj.injector_data.cloud &&
                   !unit.inj.injector_data.stochastic &&
                   dialog->findChild<QWidget*>("modelRow_Cloud_Minimum_Diameter") != nullptr &&
                   dialog->findChild<QWidget*>("modelRow_Cloud_Maximum_Diameter") != nullptr &&
                   dialog->findChild<QWidget*>("modelRow_Random_Eddy") == nullptr,
               "Cloud tracking should disable stochastic tracking and expose cloud controls"))
    {
        delete parent;
        return 1;
    }
    cloud_editor = dialog->findChild<QUI_CheckBox*>("cloudTrackingEditor");
    if (!check(cloud_editor != nullptr,
               "Cloud tracking editor should remain discoverable after rebuild"))
    {
        delete parent;
        return 1;
    }
    cloud_editor->click();
    application.processEvents();
    if (!check(dialog->findChild<QUI_CheckBox*>("stochasticTrackingEditor") != nullptr &&
                   dialog->findChild<QUI_CheckBox*>("stochasticTrackingEditor")->isEnabled(),
               "Disabling cloud tracking should re-enable stochastic tracking"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.seco_breakup_on = true;
    unit.inj.injector_data.seco_breakup_tab = true;
    unit.inj.injector_data.seco_breakup_wave = true;
    unit.inj.injector_data.seco_break_up_khrt = false;
    unit.inj.injector_data.seco_breakup_ssd = false;
    unit.inj.injector_data.seco_breakup_madahushi = false;
    unit.inj.injector_data.seco_breakup_schmehl = false;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    auto *tab_breakup_editor = dialog->findChild<QUI_CheckBox*>("secoTabulatedModelEditor");
    auto *wave_breakup_editor = dialog->findChild<QUI_CheckBox*>("secoWaveModelEditor");
    if (!check(tab_breakup_editor != nullptr && wave_breakup_editor != nullptr &&
                   unit.inj.injector_data.seco_breakup_tab &&
                   !unit.inj.injector_data.seco_breakup_wave &&
                   tab_breakup_editor->isEnabled() &&
                   !wave_breakup_editor->isEnabled() &&
                   dialog->findChild<QWidget*>("modelRow_SECO_Tabulated_Y0") != nullptr &&
                   dialog->findChild<QWidget*>("modelRow_SECO_Wave_B1") == nullptr,
               "SECO breakup should normalize to one model and expose only its parameters"))
    {
        delete parent;
        return 1;
    }
    tab_breakup_editor->click();
    application.processEvents();
    wave_breakup_editor = dialog->findChild<QUI_CheckBox*>("secoWaveModelEditor");
    if (!check(wave_breakup_editor != nullptr && wave_breakup_editor->isEnabled() &&
                   !unit.inj.injector_data.seco_breakup_tab &&
                   !unit.inj.injector_data.seco_breakup_wave,
               "Disabling the active SECO model should unlock the other models"))
    {
        delete parent;
        return 1;
    }
    wave_breakup_editor->click();
    application.processEvents();
    tab_breakup_editor = dialog->findChild<QUI_CheckBox*>("secoTabulatedModelEditor");
    if (!check(tab_breakup_editor != nullptr && !tab_breakup_editor->isEnabled() &&
                   unit.inj.injector_data.seco_breakup_wave &&
                   !unit.inj.injector_data.seco_breakup_tab &&
                   dialog->findChild<QWidget*>("modelRow_SECO_Wave_B1") != nullptr &&
                   dialog->findChild<QWidget*>("modelRow_SECO_Tabulated_Y0") == nullptr,
               "Selecting a SECO model should lock the alternatives"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.seco_breakup_on = true;
    unit.inj.injector_data.seco_breakup_tab = false;
    unit.inj.injector_data.seco_breakup_wave = false;
    unit.inj.injector_data.seco_break_up_khrt = false;
    unit.inj.injector_data.seco_breakup_ssd = false;
    unit.inj.injector_data.seco_breakup_madahushi = true;
    unit.inj.injector_data.seco_breakup_schmehl = false;
    unit.inj.injector_data.drag_law = spherical;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(unit.inj.injector_data.drag_law == dynamic_drag &&
                   dialog->findChild<QUI_CheckBox*>("secoMadabhushiModelEditor") != nullptr,
               "Madabhushi breakup should require dynamic drag"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.seco_breakup_on = false;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(unit.inj.injector_data.drag_law == spherical &&
                   !unit.inj.injector_data.seco_breakup_madahushi,
               "Disabling breakup should clear Madabhushi and dynamic drag"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.seco_breakup_on = false;
    unit.inj.injector_data.drag_law = spherical;
    unit.inj.injector_data.brownian_motion = true;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    auto *brownian_row = dialog->findChild<QWidget*>("modelRow_Brownian_Motion");
    if (!check(brownian_row != nullptr && !brownian_row->isEnabled() &&
                   !unit.inj.injector_data.brownian_motion,
               "Brownian motion should be disabled outside Stokes-Cunningham drag"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.drag_law = Strokes_Cunningham;
    unit.inj.injector_data.brownian_motion = true;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    brownian_row = dialog->findChild<QWidget*>("modelRow_Brownian_Motion");
    if (!check(brownian_row != nullptr && brownian_row->isEnabled() &&
                   unit.inj.injector_data.brownian_motion,
               "Brownian motion should be available with Stokes-Cunningham drag"))
    {
        delete parent;
        return 1;
    }

    dialog->set_chemkin_species_names({"CO2", "H2O"});
    if (!check(devolatilizing_species_editor->count() == 3 &&
                   devolatilizing_species_editor->itemText(1) == "CO2" &&
                   devolatilizing_species_editor->itemText(2) == "H2O" &&
                   evaporating_species_editor->count() == 3,
               "Open unit editor should refresh species options after Chemkin changes"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.evaporating_liquid = true;
    unit.inj.injector_data.evaporating_material = "water";
    dialog->refresh_from_unit_data(&unit);
    auto *evaporating_material_editor =
        dialog->findChild<QUI_ComboBox*>("evaporatingMaterialModelEditor");
    if (!check(evaporating_material_editor != nullptr,
               "Wet-combustion material editor should be available when enabled"))
    {
        delete parent;
        return 1;
    }
    dialog->set_material_names({"ethanol", "water"});
    if (!check(evaporating_material_editor->count() == 2 &&
                   evaporating_material_editor->itemText(0) == "ethanol" &&
                   evaporating_material_editor->itemText(1) == "water",
               "Open unit editor should refresh material context without stale options"))
    {
        delete parent;
        return 1;
    }

    dialog->set_material_names({"ethanol"});
    if (!check(evaporating_material_editor->currentText() == "water" &&
                   evaporating_material_editor->findText("water") < 0 &&
                   evaporating_material_editor->findText("ethanol") >= 0 &&
                   evaporating_material_editor->lineEdit() != nullptr &&
                   evaporating_material_editor->lineEdit()->isReadOnly(),
               "Removed material should remain visible but not selectable"))
    {
        delete parent;
        return 1;
    }

    bool cancel_signal_received = false;
    QObject::connect(dialog, &unit_edit_dialog::dialog_cancelled,
                     [&cancel_signal_received](Unit *)
    {
        cancel_signal_received = true;
    });
    auto *name_editor = dialog->findChild<QUI_LineEdit*>("injectionNameEditor");
    if (!check(name_editor != nullptr,
               "Unit editor should expose its injection name editor"))
    {
        delete parent;
        return 1;
    }
    name_editor->setText("cancelled-edit");
    name_editor->commit();
    if (!check(dialog->has_unsaved_changes(),
               "Committed editor input should mark the dialog modified"))
    {
        delete parent;
        return 1;
    }
    dialog->findChild<QPushButton*>("cancelChangesButton")->click();
    application.processEvents();
    if (!check(cancel_signal_received && !dialog->isVisible() &&
                   !dialog->has_unsaved_changes(),
               "Cancel Changes should emit its signal and close the editor"))
    {
        delete parent;
        return 1;
    }
    dialog->show();
    application.processEvents();

    Injector_OCCT geometry;
    geometry.injector_data.vel = QVector3D(0.0f, 0.0f, 1.0f);
    geometry.injector_data.total_flow_rate = 1.0;
    if (!check(geometry.create_injector(),
               "A valid injector should create geometry") ||
        !check(!geometry.shape.IsNull(),
               "A valid injector should retain its geometry"))
    {
        delete parent;
        return 1;
    }

    geometry.injector_data.vel = QVector3D();
    if (!check(!geometry.create_injector(),
               "An invalid injector should fail geometry creation") ||
        !check(!geometry.shape.IsNull(),
               "A failed rebuild should preserve the previous geometry"))
    {
        delete parent;
        return 1;
    }

    dialog->close();
    application.processEvents();
    if (!check(dialog != nullptr,
               "Unit editor should remain owned after close without delete-on-close"))
    {
        delete parent;
        return 1;
    }

    delete parent;
    application.processEvents();
    return check(dialog == nullptr,
                 "Unit editor should be destroyed safely with its parent")
               ? 0
               : 1;
}
