#include "unit_edit_dialog.h"

#include <QApplication>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QStandardItemModel>
#include <QTabWidget>
#include <QWidget>

#include <iostream>

namespace
{
bool check(bool condition, const char *message)
{
    if (!condition)
    {
        qCritical() << message;
        std::cerr << message << std::endl;
        return false;
    }
    return true;
}

bool has_field_row(const QObject *root, const QString &label)
{
    if (root == nullptr)
    {
        return false;
    }

    QList<const QObject *> pending = {root};
    while (!pending.isEmpty())
    {
        const QObject *object = pending.takeLast();
        if (const auto *row = dynamic_cast<const QUI_FieldRow *>(object);
            row != nullptr && row->label_text() == label)
        {
            return true;
        }

        for (const QObject *child : object->children())
        {
            pending.push_back(child);
        }
    }
    return false;
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
    auto *oxidizing_species_editor =
        dialog->findChild<QUI_ComboBox*>("oxidizingSpeciesEditor");
    auto *product_species_editor =
        dialog->findChild<QUI_ComboBox*>("productSpeciesEditor");
    if (!check(material_editor != nullptr && diameter_editor != nullptr &&
                   evaporating_species_editor != nullptr &&
                   devolatilizing_species_editor != nullptr &&
                   oxidizing_species_editor != nullptr &&
                   product_species_editor != nullptr,
               "Particle-dependent editors should be discoverable"))
    {
        delete parent;
        return 1;
    }

    Unit_Edit_Case_Context chemistry_context;
    chemistry_context.energy_equation = Unit_Edit_Feature_State::Enabled;
    chemistry_context.active_chemistry_species_count = -1;
    chemistry_context.nonpremixed_combustion = Unit_Edit_Feature_State::Disabled;
    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.injection_type = group;
    dialog->set_case_context(chemistry_context);
    dialog->set_chemkin_species_names({"O2"});
    application.processEvents();
    if (!check(unit.inj.injector_data.type == Inert,
               "Chemkin species count should enforce the minimum chemistry prerequisite"))
    {
        delete parent;
        return 1;
    }
    dialog->set_chemkin_species_names({"O2", "N2"});
    unit.inj.injector_data.type = Droplet;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.type == Droplet,
               "A sufficient Chemkin species count should keep Droplet available"))
    {
        delete parent;
        return 1;
    }
    chemistry_context.energy_equation = Unit_Edit_Feature_State::Unknown;
    chemistry_context.heat_transfer = Unit_Edit_Feature_State::Disabled;
    chemistry_context.active_chemistry_species_count = 2;
    unit.inj.injector_data.type = Droplet;
    dialog->set_case_context(chemistry_context);
    if (!check(unit.inj.injector_data.type == Inert,
               "Disabled heat transfer should restrict heat-dependent particle types"))
    {
        delete parent;
        return 1;
    }
    chemistry_context.heat_transfer = Unit_Edit_Feature_State::Unknown;
    chemistry_context.active_chemistry_species_count = -1;
    unit.inj.injector_data.type = Droplet;
    dialog->set_case_context(chemistry_context);
    dialog->set_chemkin_species_names({});
    if (!check(unit.inj.injector_data.type == Droplet,
               "Clearing Chemkin species should remove the stale automatic count"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = condensate;
    chemistry_context.energy_equation = Unit_Edit_Feature_State::Unknown;
    chemistry_context.heat_transfer = Unit_Edit_Feature_State::Disabled;
    dialog->set_case_context(chemistry_context);
    if (!check(unit.inj.injector_data.injection_type == single,
               "Condensate should require heat transfer, not only the energy equation"))
    {
        delete parent;
        return 1;
    }
    auto *heat_injection_type_editor =
        dialog->findChild<QUI_ComboBox*>("injectionTypeEditor");
    bool condensate_disabled = false;
    if (heat_injection_type_editor != nullptr)
    {
        const int condensate_index = heat_injection_type_editor->findData(condensate);
        if (condensate_index >= 0)
        {
            const auto *model = qobject_cast<const QStandardItemModel *>(
                heat_injection_type_editor->model());
            condensate_disabled = model != nullptr && model->item(condensate_index) != nullptr &&
                !model->item(condensate_index)->isEnabled();
        }
    }
    if (!check(condensate_disabled,
               "Condensate should be disabled in the injection-type list without heat transfer"))
    {
        delete parent;
        return 1;
    }
    chemistry_context.heat_transfer = Unit_Edit_Feature_State::Unknown;
    dialog->set_chemkin_species_names({"O2", "N2"});
    unit.inj.injector_data.injection_type = single;
    dialog->refresh_from_unit_data(&unit);

    auto *collision_partner_row = dialog->findChild<QWidget*>("modelRow_Collision_Partner");
    auto *continuous_phase_domain_row =
        dialog->findChild<QWidget*>("modelRow_Continuous_Phase_Domain");
    if (!check(collision_partner_row != nullptr && continuous_phase_domain_row != nullptr &&
                   !collision_partner_row->isEnabled() &&
                   !continuous_phase_domain_row->isEnabled(),
               "DDPM-only physical-model fields should be locked without a DPM domain"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.type = Inert;
    unit.inj.injector_data.dpm_domain = "secondary-domain";
    dialog->refresh_from_unit_data(&unit);
    collision_partner_row = dialog->findChild<QWidget*>("modelRow_Collision_Partner");
    continuous_phase_domain_row =
        dialog->findChild<QWidget*>("modelRow_Continuous_Phase_Domain");
    if (!check(collision_partner_row != nullptr && continuous_phase_domain_row != nullptr &&
                   collision_partner_row->isEnabled() &&
                   continuous_phase_domain_row->isEnabled(),
               "DDPM-only physical-model fields should unlock for a DPM domain"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.dpm_domain = "none";
    dialog->refresh_from_unit_data(&unit);

    unit.inj.injector_data.dpm_domain = "secondary-domain";
    unit.inj.injector_data.drag_law = wen_yu;
    Unit_Edit_Case_Context drag_context;
    drag_context.dense_gas_solid = Unit_Edit_Feature_State::Disabled;
    dialog->set_case_context(drag_context);
    if (!check(unit.inj.injector_data.drag_law == spherical &&
                   dialog->findChild<QUI_ComboBox*>("modelEditor_Drag_Law") != nullptr &&
                   dialog->findChild<QUI_ComboBox*>("modelEditor_Drag_Law")->findData(wen_yu) < 0,
               "Dense gas-solid drag laws should be removed without a dense-flow context"))
    {
        delete parent;
        return 1;
    }
    drag_context.dense_gas_solid = Unit_Edit_Feature_State::Enabled;
    dialog->set_case_context(drag_context);
    auto *dense_drag_editor = dialog->findChild<QUI_ComboBox*>("modelEditor_Drag_Law");
    if (!check(dense_drag_editor != nullptr && dense_drag_editor->findData(wen_yu) >= 0,
               "Dense gas-solid drag laws should be available in DDPM dense flow"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.type = Droplet;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.drag_law == spherical &&
                   dialog->findChild<QUI_ComboBox*>("modelEditor_Drag_Law") != nullptr &&
                   dialog->findChild<QUI_ComboBox*>("modelEditor_Drag_Law")->findData(wen_yu) < 0,
               "Dense gas-solid drag laws should require inert particles"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.type = Inert;
    dialog->refresh_from_unit_data(&unit);
    unit.inj.injector_data.drag_law = grace;
    drag_context.gravity = Unit_Edit_Feature_State::Disabled;
    dialog->set_case_context(drag_context);
    if (!check(unit.inj.injector_data.drag_law == spherical,
               "Grace drag law should fall back when gravity is disabled"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.dpm_domain = "none";
    dialog->set_case_context(Unit_Edit_Case_Context{});

    auto *number_of_stream_spin = dialog->findChild<QUI_SpinBox *>();
    if (!check(number_of_stream_spin != nullptr &&
                   !number_of_stream_spin->isVisible(),
               "Single injections should not expose Number of Streams"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.stagger_radius = -1.0;
    unit.inj.injector_data.spatial_staggering_std_inj_on = false;
    dialog->refresh_from_unit_data(&unit);
    auto *stagger_radius_editor = dialog->findChild<QUI_LineEdit*>("staggerRadiusEditor");
    if (!check(stagger_radius_editor != nullptr &&
                   unit.inj.injector_data.stagger_radius == 0.0 &&
                   !stagger_radius_editor->isEnabled(),
               "Stagger radius should be non-negative and locked when disabled"))
    {
        delete parent;
        return 1;
    }

    // Use Group explicitly so the distribution expectation does not depend on
    // Injector's constructor defaults.
    unit.inj.injector_data.injection_type = cone;
    unit.inj.injector_data.cone_type = solid;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(number_of_stream_spin != nullptr &&
                   number_of_stream_spin->isVisible(),
               "Generic injection types should expose Number of Streams"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.numpts = 0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.numpts == 1 &&
                   number_of_stream_spin->value() == 1,
               "Number of Streams should be normalized to a positive value"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Massless;
    unit.inj.injector_data.injection_type = volume;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(!number_of_stream_spin->isVisible(),
               "Volume should use its dedicated stream specification instead of Number of Streams"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = surface;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(number_of_stream_spin->isVisible(),
               "Surface injections should expose Number of Streams"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = group;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(number_of_stream_spin->isVisible(),
               "Leaving Volume should restore Number of Streams"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Inert;
    unit.inj.injector_data.injection_type = group;
    unit.inj.injector_data.stochastic = true;
    unit.inj.injector_data.random_eddy = false;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    auto *random_eddy_attempts_row = dialog->findChild<QWidget*>("modelRow_Eddy_Attempts");
    auto *time_scale_row = dialog->findChild<QWidget*>("modelRow_Time_Scale_Constant");
    if (!check(random_eddy_attempts_row != nullptr && random_eddy_attempts_row->isEnabled() &&
                   time_scale_row != nullptr && !time_scale_row->isEnabled(),
               "Random Eddy should control the Time Scale Constant without disabling Eddy Attempts"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Massless;
    unit.inj.injector_data.cloud = true;
    unit.inj.injector_data.stochastic = false;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    auto *cloud_row = dialog->findChild<QWidget*>("modelRow_Cloud_Tracking");
    if (!check(!unit.inj.injector_data.cloud &&
                   cloud_row != nullptr && !cloud_row->isEnabled(),
               "Massless particles should not allow Cloud Tracking"))
    {
        delete parent;
        return 1;
    }

    const QList<QPair<DPM_Type, QList<bool>>> particle_expectations = {
        {Massless, {false, false, false, false, false, false}},
        {Inert, {false, true, false, false, false, false}},
        {Droplet, {true, true, true, false, false, false}},
        {Combusting, {true, true, false, true, true, true}},
        {Multicomponent, {true, true, false, false, false, false}}
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
            devolatilizing_species_editor->isEnabled(),
            oxidizing_species_editor->isEnabled(),
            product_species_editor->isEnabled()
        };
        if (!check(actual == expectation.second,
                   "Particle-dependent editor enablement is incorrect"))
        {
            delete parent;
            return 1;
        }
    }

    for (const DPM_Type physical_type : {Inert, Droplet, Combusting, Multicomponent})
    {
        unit.inj.injector_data.type = physical_type;
        dialog->refresh_from_unit_data(&unit);
        if (!check(diameter_editor->isEnabled(),
                   "Physical particles should support group diameter distributions"))
        {
            delete parent;
            return 1;
        }
    }

    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.material = "water";
    unit.inj.injector_data.evaporating_species = "H2O";
    unit.inj.injector_data.devolatilizing_species = "volatile";
    unit.inj.injector_data.oxidizing_species = "O2";
    unit.inj.injector_data.product_species = "CO2";
    dialog->refresh_from_unit_data(&unit);
    unit.inj.injector_data.type = Massless;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.material.isEmpty() &&
                   unit.inj.injector_data.evaporating_species.isEmpty() &&
                   unit.inj.injector_data.devolatilizing_species.isEmpty() &&
                   unit.inj.injector_data.oxidizing_species.isEmpty() &&
                   unit.inj.injector_data.product_species.isEmpty(),
               "Unsupported particle fields should be cleared on type changes"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.rr_disturb = true;
    unit.inj.injector_data.rr_min = 4.0;
    unit.inj.injector_data.rr_max = 1.0;
    unit.inj.injector_data.rr_mean = 20.0;
    unit.inj.injector_data.rr_spread = 0.0;
    unit.inj.injector_data.rr_numdia = 0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.rr_max == 4.0 &&
                   unit.inj.injector_data.rr_mean == 4.0 &&
                   unit.inj.injector_data.rr_spread > 0.0 &&
                   unit.inj.injector_data.rr_numdia == 1,
               "Rosin-Rammler parameters should be normalized to valid bounds"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = cone;
    unit.inj.injector_data.cone_type = solid;
    unit.inj.injector_data.tabulated_diam_dist = true;
    unit.inj.injector_data.rr_disturb = true;
    unit.inj.injector_data.rr_uniform_ln_d = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.tabulated_diam_dist &&
                   !unit.inj.injector_data.rr_disturb &&
                   !unit.inj.injector_data.rr_uniform_ln_d,
               "Tabulated diameter distribution should clear RR flags"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.tabulated_diam_dist = false;
    unit.inj.injector_data.rr_disturb = false;
    unit.inj.injector_data.rr_uniform_ln_d = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.rr_disturb &&
                   unit.inj.injector_data.rr_uniform_ln_d,
               "Logarithmic RR should enable the RR distribution flag"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.injection_type = single;
    unit.inj.injector_data.single_direction_mode = Single_Direction_Mode::Vector;
    dialog->refresh_from_unit_data(&unit);
    auto *single_direction_editor = dialog->findChild<QUI_ComboBox *>(
        "propertyEditor_Direction_Mode");
    if (!check(single_direction_editor != nullptr &&
                   has_field_row(dialog, "X-Velocity") &&
                   !has_field_row(dialog, "Pitch") &&
                   !has_field_row(dialog, "X-Target Hitpoint"),
               "Single vector mode should expose velocity components"))
    {
        delete parent;
        return 1;
    }
    single_direction_editor->setCurrentIndex(
        static_cast<int>(Single_Direction_Mode::Pitch_Yaw));
    single_direction_editor->selection_committed();
    application.processEvents();
    if (!check(unit.inj.injector_data.single_direction_mode ==
                   Single_Direction_Mode::Pitch_Yaw &&
                   has_field_row(dialog, "Pitch") &&
                   has_field_row(dialog, "Yaw") &&
                   !has_field_row(dialog, "X-Velocity"),
               "Single pitch-yaw mode should expose angle components"))
    {
        delete parent;
        return 1;
    }
    single_direction_editor = dialog->findChild<QUI_ComboBox *>(
        "propertyEditor_Direction_Mode");
    single_direction_editor->setCurrentIndex(
        static_cast<int>(Single_Direction_Mode::Target_Hitpoint));
    single_direction_editor->selection_committed();
    application.processEvents();
    if (!check(has_field_row(dialog, "X-Target Hitpoint") &&
                   has_field_row(dialog, "Y-Target Hitpoint") &&
                   has_field_row(dialog, "Z-Target Hitpoint") &&
                   dialog->findChild<QUI_ComboBox *>(
                       "propertyEditor_Target_Scope") != nullptr &&
                   !has_field_row(dialog, "Pitch"),
               "Single target mode should expose target coordinates"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Massless;
    unit.inj.injector_data.injection_type = single;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "X-Position") &&
                   !has_field_row(dialog, "X-Velocity") &&
                   !has_field_row(dialog, "Diameter") &&
                   !has_field_row(dialog, "Temperature") &&
                   !has_field_row(dialog, "Flow Rate"),
               "Massless single injections should expose position only"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = group;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "X-Position") &&
                   !has_field_row(dialog, "X-Velocity") &&
                   !has_field_row(dialog, "Diameter"),
               "Massless group injections should expose positions only"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = surface;
    unit.inj.injector_data.use_face_normal = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "Surface Zone IDs") &&
                   !has_field_row(dialog, "X-Position") &&
                   !has_field_row(dialog, "X-Velocity") &&
                   !has_field_row(dialog, "Diameter") &&
                   !has_field_row(dialog, "Temperature") &&
                   !has_field_row(dialog, "Flow Rate") &&
                   !has_field_row(dialog, "Use Face Normal") &&
                   !unit.inj.injector_data.use_face_normal,
               "Massless surface injections should expose surface selection only"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = volume;
    dialog->refresh_from_unit_data(&unit);
    if (!check(!has_field_row(dialog, "Volume Fraction"),
               "Massless volume injections should hide volume fraction"))
    {
        delete parent;
        return 1;
    }
    if (!check(!has_field_row(dialog, "Packing Limit"),
               "Massless volume injections should hide packing limit"))
    {
        delete parent;
        return 1;
    }
    if (!check(!has_field_row(dialog, "Mass Input") &&
                   !has_field_row(dialog, "Volume Fraction Input"),
               "Massless volume injections should hide particle amount inputs"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.local_reference_frame = "rotating-frame";
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.local_reference_frame == "global",
               "Surface and volume injections should clear unsupported local reference frames"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = condensate;
    unit.inj.injector_data.local_reference_frame = "rotating-frame";
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.local_reference_frame == "global",
               "Condensate injections should clear unsupported local reference frames"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Inert;
    unit.inj.injector_data.injection_type = volume;
    unit.inj.injector_data.volume_specification = bouning_geometry;
    unit.inj.injector_data.volume_streams_spec = parcel_per_cell;
    unit.inj.injector_data.volume_streams_total = 0;
    unit.inj.injector_data.volume_streams_per_cell = 0;
    dialog->refresh_from_unit_data(&unit);
    auto *stream_specification_editor = dialog->findChild<QUI_ComboBox *>(
        "propertyEditor_Stream_Specification");
    if (!check(unit.inj.injector_data.volume_streams_spec == total_parcel_count &&
                   unit.inj.injector_data.volume_streams_total == 1 &&
                   unit.inj.injector_data.volume_streams_per_cell == 1 &&
                   stream_specification_editor != nullptr &&
                   stream_specification_editor->count() == 1 &&
                   stream_specification_editor->currentText() == "Total Parcel Count",
               "Bounding-geometry volume injections should use starting-point streams only"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Massless;
    unit.inj.injector_data.injection_type = cone;
    unit.inj.injector_data.cone_type = ring;
    unit.inj.injector_data.uniform_mass_dist_on = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(!unit.inj.injector_data.uniform_mass_dist_on &&
                   has_field_row(dialog, "X-Axis") &&
                   has_field_row(dialog, "Cone Angle") &&
                   has_field_row(dialog, "Outer Radius") &&
                   has_field_row(dialog, "Inner Radius") &&
                   !has_field_row(dialog, "Velocity Magnitude") &&
                   !has_field_row(dialog, "Total Flow Rate") &&
                   dialog->findChild<QWidget*>("modelRow_Uniform_Mass_Distribution") == nullptr,
               "Massless cone injections should expose geometry but not mass properties"))
    {
        delete parent;
        return 1;
    }

    QComboBox *cone_type_editor = nullptr;
    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.cone_type = solid;
    unit.inj.injector_data.uniform_mass_dist_on = true;
    unit.inj.injector_data.spatial_staggering_std_inj_on = false;
    unit.inj.injector_data.spatial_staggering_atomizer_on = true;
    dialog->refresh_from_unit_data(&unit);
    cone_type_editor = dialog->findChild<QComboBox*>("comboBox_conetype");
    if (!check(cone_type_editor != nullptr &&
                   dialog->findChild<QWidget*>("modelRow_Uniform_Mass_Distribution") != nullptr,
               "Solid cone injections should expose uniform mass distribution"))
    {
        delete parent;
        return 1;
    }
    auto *stagger_editor = dialog->findChild<QUI_CheckBox*>("staggerOptionsEditor");
    if (!check(stagger_editor != nullptr &&
                   stagger_editor->isChecked() &&
                   !unit.inj.injector_data.spatial_staggering_std_inj_on &&
                   stagger_radius_editor != nullptr &&
                   !stagger_radius_editor->isEnabled(),
               "Solid-cone staggering should use the atomizer staggering option"))
    {
        delete parent;
        return 1;
    }
    auto *atomizer_stagger_row = dialog->findChild<QWidget*>("modelRow_Atomizer_Staggering");
    auto *atomizer_stagger_editor = atomizer_stagger_row != nullptr
        ? atomizer_stagger_row->findChild<QUI_CheckBox*>()
        : nullptr;
    if (!check(atomizer_stagger_editor != nullptr && atomizer_stagger_editor->isChecked(),
               "Physical Models should mirror the top-level stagger option"))
    {
        delete parent;
        return 1;
    }
    atomizer_stagger_editor->click();
    application.processEvents();
    if (!check(!unit.inj.injector_data.spatial_staggering_atomizer_on &&
                   !stagger_editor->isChecked(),
               "Disabling model staggering should update the top-level option"))
    {
        delete parent;
        return 1;
    }
    stagger_editor->click();
    application.processEvents();
    if (!check(unit.inj.injector_data.spatial_staggering_atomizer_on &&
                   atomizer_stagger_editor->isChecked(),
               "Enabling the top-level stagger option should update Physical Models"))
    {
        delete parent;
        return 1;
    }
    for (int index = 0; index < cone_type_editor->count(); ++index)
    {
        if (cone_type_editor->itemData(index).toInt() == point)
        {
            cone_type_editor->setCurrentIndex(index);
            break;
        }
    }
    application.processEvents();
    if (!check(!unit.inj.injector_data.uniform_mass_dist_on &&
                   !has_field_row(dialog, "Outer Radius") &&
                   !has_field_row(dialog, "Inner Radius") &&
                   dialog->findChild<QWidget*>("modelRow_Uniform_Mass_Distribution") == nullptr,
               "Point cones should hide radius fields and dependent options"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.cone_type = hollow;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "Outer Radius") &&
                   !has_field_row(dialog, "Inner Radius"),
               "Hollow cones should expose only outer radius"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.cone_type = ring;
    unit.inj.injector_data.radius = 10.0;
    unit.inj.injector_data.inner_radius = 20.0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "Outer Radius") &&
                   unit.inj.injector_data.inner_radius == 9.5 &&
                   has_field_row(dialog, "Inner Radius"),
               "Ring cones should expose valid inner and outer radii"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.cone_angle = 200.0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.cone_angle < 180.0,
               "Cone angle should remain below Fluent's 180-degree limit"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.swirl_frac = 0.4;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.swirl_frac == 0.0 &&
                   dialog->findChild<QWidget*>("modelRow_Swirl_Fraction") == nullptr,
               "Ring cones should not expose hollow-cone swirl fraction"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.cone_type = hollow;
    unit.inj.injector_data.swirl_frac = -0.4;
    dialog->refresh_from_unit_data(&unit);
    if (!check(dialog->findChild<QWidget*>("modelRow_Swirl_Fraction") != nullptr &&
                   unit.inj.injector_data.swirl_frac == -0.4,
               "Hollow cones should expose swirl fraction"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = group;
    unit.inj.injector_data.spatial_staggering_std_inj_on = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(stagger_radius_editor->isEnabled(),
               "Standard injection staggering should allow Stagger Radius"))
    {
        delete parent;
        return 1;
    }

    cone_type_editor = dialog->findChild<QComboBox*>("comboBox_conetype");
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

    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.injection_type = volume;
    unit.inj.injector_data.tabulated_diam_dist = true;
    unit.inj.injector_data.rr_disturb = false;
    unit.inj.injector_data.rr_uniform_ln_d = false;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(diameter_editor->findData(3) >= 0 &&
                   unit.inj.injector_data.tabulated_diam_dist,
               "Volume injections should support tabulated diameter distributions"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = cone;
    unit.inj.injector_data.tabulated_diam_dist = true;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(diameter_editor->findData(3) >= 0 &&
                   diameter_editor->currentData().toInt() == 3 &&
                   unit.inj.injector_data.tabulated_diam_dist,
               "Cone injections should support tabulated diameter distributions"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = surface;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(diameter_editor->findData(3) >= 0,
               "Surface injections should support tabulated diameter distributions"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.spatial_staggering_std_inj_on = true;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(!unit.inj.injector_data.spatial_staggering_std_inj_on &&
                   !stagger_editor->isEnabled() &&
                   stagger_radius_editor != nullptr &&
                   !stagger_radius_editor->isEnabled() &&
                   dialog->findChild<QWidget*>("modelRow_Standard_Injection_Staggering") == nullptr,
               "Surface injections should use Random Surface instead of generic staggering"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = group;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(diameter_editor->findData(3) < 0 &&
                   !unit.inj.injector_data.tabulated_diam_dist,
               "Unsupported tabulated distributions should be cleared on injection-type change"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = volume;
    unit.inj.injector_data.spatial_staggering_std_inj_on = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(!unit.inj.injector_data.spatial_staggering_std_inj_on &&
                   !stagger_editor->isEnabled() &&
                   dialog->findChild<QWidget*>("modelRow_Standard_Injection_Staggering") == nullptr,
               "Volume injections should not expose generic staggering"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.mass_input_on = true;
    unit.inj.injector_data.volfrac_input_on = true;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    auto *mass_input_editor = dialog->findChild<QUI_CheckBox*>("volumeMassInputEditor");
    auto *volume_fraction_input_editor =
        dialog->findChild<QUI_CheckBox*>("volumeFractionInputEditor");
    if (!check(mass_input_editor != nullptr && volume_fraction_input_editor != nullptr &&
                   unit.inj.injector_data.mass_input_on &&
                   !unit.inj.injector_data.volfrac_input_on &&
                   mass_input_editor->isEnabled() &&
                   !volume_fraction_input_editor->isEnabled(),
               "Volume mass and volume-fraction inputs should be mutually exclusive"))
    {
        delete parent;
        return 1;
    }
    mass_input_editor->click();
    application.processEvents();
    volume_fraction_input_editor =
        dialog->findChild<QUI_CheckBox*>("volumeFractionInputEditor");
    if (!check(volume_fraction_input_editor != nullptr &&
                   !unit.inj.injector_data.mass_input_on &&
                   volume_fraction_input_editor->isEnabled(),
               "Disabling mass input should unlock volume-fraction input"))
    {
        delete parent;
        return 1;
    }
    volume_fraction_input_editor->click();
    application.processEvents();
    mass_input_editor = dialog->findChild<QUI_CheckBox*>("volumeMassInputEditor");
    if (!check(mass_input_editor != nullptr &&
                   unit.inj.injector_data.volfrac_input_on &&
                   !unit.inj.injector_data.mass_input_on &&
                   !mass_input_editor->isEnabled(),
               "Selecting volume fraction should lock mass input"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.mass_input_on = false;
    unit.inj.injector_data.volfrac_input_on = false;
    dialog->refresh_from_unit_data(&unit);
    const QObject *point_properties = dialog->findChild<QWidget *>(
        "scrollarea_properties");
    if (!check(point_properties != nullptr &&
                   has_field_row(point_properties, "Total Flow Rate") &&
                   !has_field_row(point_properties, "Total Mass") &&
                   !has_field_row(point_properties, "Volume Fraction"),
               "Volume injections should show Total Flow Rate by default"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.mass_input_on = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(point_properties, "Total Mass") &&
                   !has_field_row(point_properties, "Total Flow Rate") &&
                   !has_field_row(point_properties, "Volume Fraction"),
               "Mass input mode should show Total Mass only"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.mass_input_on = false;
    unit.inj.injector_data.volfrac_input_on = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(point_properties, "Volume Fraction") &&
                   !has_field_row(point_properties, "Total Flow Rate") &&
                   !has_field_row(point_properties, "Total Mass"),
               "Volume-fraction input mode should show Volume Fraction only"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = surface;
    unit.inj.injector_data.scale_by_area = true;
    unit.inj.injector_data.random_surface = true;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    auto *scale_area_editor = dialog->findChild<QUI_CheckBox*>("surfaceScaleByAreaEditor");
    auto *random_surface_editor = dialog->findChild<QUI_CheckBox*>("randomSurfaceEditor");
    if (!check(scale_area_editor != nullptr && random_surface_editor != nullptr &&
                   unit.inj.injector_data.scale_by_area &&
                   !unit.inj.injector_data.random_surface &&
                   scale_area_editor->isEnabled() &&
                   !random_surface_editor->isEnabled(),
               "Surface distribution options should normalize and lock each other"))
    {
        delete parent;
        return 1;
    }
    scale_area_editor->click();
    application.processEvents();
    scale_area_editor = dialog->findChild<QUI_CheckBox*>("surfaceScaleByAreaEditor");
    random_surface_editor = dialog->findChild<QUI_CheckBox*>("randomSurfaceEditor");
    if (!check(random_surface_editor != nullptr && random_surface_editor->isEnabled() &&
                   !unit.inj.injector_data.scale_by_area &&
                   !unit.inj.injector_data.random_surface &&
                   scale_area_editor != nullptr && scale_area_editor->isEnabled(),
               "Disabling face-area scaling should unlock random surface placement"))
    {
        delete parent;
        return 1;
    }
    random_surface_editor->click();
    application.processEvents();
    scale_area_editor = dialog->findChild<QUI_CheckBox*>("surfaceScaleByAreaEditor");
    if (!check(scale_area_editor != nullptr && !scale_area_editor->isEnabled() &&
                   unit.inj.injector_data.random_surface &&
                   !unit.inj.injector_data.scale_by_area,
               "Random surface placement should lock face-area scaling"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.use_face_normal = false;
    dialog->refresh_from_unit_data(&unit);
    auto *face_normal_row = dialog->findChild<QWidget*>("modelRow_Use_Face_Normal");
    auto *face_normal_editor = face_normal_row != nullptr
        ? face_normal_row->findChild<QUI_CheckBox*>()
        : nullptr;
    if (!check(face_normal_editor != nullptr &&
                   has_field_row(dialog, "X-Velocity") &&
                   !has_field_row(dialog, "Velocity Magnitude"),
               "Surface injections should expose velocity components by default"))
    {
        delete parent;
        return 1;
    }
    face_normal_editor->click();
    application.processEvents();
    if (!check(!has_field_row(dialog, "X-Velocity") &&
                   has_field_row(dialog, "Velocity Magnitude"),
               "Face-normal surface injections should use velocity magnitude"))
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
    cloud_editor = dialog->findChild<QUI_CheckBox*>("cloudTrackingEditor");
    if (cloud_editor != nullptr)
    {
        cloud_editor->click();
    }
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
    unit.inj.injector_data.cloud_min_dia = 10.0;
    unit.inj.injector_data.cloud_max_dia = 1.0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.cloud_max_dia == 10.0,
               "Cloud diameter limits should be normalized into a valid range"))
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

    unit.inj.injector_data.injection_type = file_;
    unit.inj.injector_data.unsteady_start = -2.0;
    unit.inj.injector_data.unsteady_stop = -1.0;
    unit.inj.injector_data.start_at_flow_time_in_unsteady_inj_file = -3.0;
    unit.inj.injector_data.interval_to_repeat_in_unsteady_inj_file = -4.0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.unsteady_start == 0.0 &&
                   unit.inj.injector_data.unsteady_stop == 0.0 &&
                   unit.inj.injector_data.start_at_flow_time_in_unsteady_inj_file == 0.0 &&
                   unit.inj.injector_data.interval_to_repeat_in_unsteady_inj_file == 0.0 &&
                   has_field_row(dialog, "Start Time") &&
                   has_field_row(dialog, "Stop Time") &&
                   has_field_row(dialog, "Start Flow-Time in File") &&
                   has_field_row(dialog, "Repeat Interval in File") &&
                   !has_field_row(dialog, "X-Position") &&
                   !has_field_row(dialog, "X-Velocity"),
               "File injections should expose only file and timing controls"))
    {
        delete parent;
        return 1;
    }
    cloud_editor = dialog->findChild<QUI_CheckBox*>("cloudTrackingEditor");
    if (cloud_editor != nullptr)
    {
        cloud_editor->click();
    }
    application.processEvents();
    if (!check(dialog->findChild<QUI_CheckBox*>("stochasticTrackingEditor") != nullptr &&
                   dialog->findChild<QUI_CheckBox*>("stochasticTrackingEditor")->isEnabled(),
               "Disabling cloud tracking should re-enable stochastic tracking"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.stochastic = false;
    unit.inj.injector_data.cloud = true;
    unit.inj.injector_data.random_eddy = true;
    unit.inj.injector_data.ntries = 6;
    dialog->refresh_from_unit_data(&unit);
    if (!check(!unit.inj.injector_data.random_eddy &&
                   unit.inj.injector_data.ntries == 1,
               "Disabling stochastic tracking should clear dependent eddy settings"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.injection_type = condensate;
    unit.inj.injector_data.rr_disturb = true;
    unit.inj.injector_data.rr_uniform_ln_d = true;
    unit.inj.injector_data.unsteady_start = -2.0;
    unit.inj.injector_data.unsteady_stop = -1.0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.unsteady_start == 0.0 &&
                   unit.inj.injector_data.unsteady_stop == 0.0 &&
                   !unit.inj.injector_data.rr_disturb &&
                   !unit.inj.injector_data.rr_uniform_ln_d &&
                   has_field_row(dialog, "Start Time") &&
                   has_field_row(dialog, "Stop Time") &&
                   !has_field_row(dialog, "X-Position") &&
                   !has_field_row(dialog, "X-Velocity") &&
                   !has_field_row(dialog, "Radius"),
               "Condensate injections should expose only a valid time interval"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Inert;
    unit.inj.injector_data.injection_type = condensate;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    const QList<QRadioButton *> particle_buttons =
        dialog->findChildren<QRadioButton *>();
    bool massless_enabled = true;
    bool inert_enabled = true;
    bool droplet_enabled = false;
    bool combusting_enabled = true;
    bool multicomponent_enabled = false;
    for (QRadioButton *button : particle_buttons)
    {
        if (button == nullptr)
        {
            continue;
        }
        if (button->text() == "Massless") massless_enabled = button->isEnabled();
        if (button->text() == "Inert") inert_enabled = button->isEnabled();
        if (button->text() == "Droplet") droplet_enabled = button->isEnabled();
        if (button->text() == "Combusting") combusting_enabled = button->isEnabled();
        if (button->text() == "Multicomponent") multicomponent_enabled = button->isEnabled();
    }
    if (!check(unit.inj.injector_data.type == Droplet &&
                   !massless_enabled && !inert_enabled && droplet_enabled &&
                   !combusting_enabled && multicomponent_enabled,
               "Condensate should restrict particle type to Droplet or Multicomponent"))
    {
        delete parent;
        return 1;
    }

    if (!check(dialog->findChild<QWidget*>("modelRow_Local_Reference_Frame") == nullptr,
               "Condensate should not expose a local reference-frame selector"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = surface;
    dialog->refresh_from_unit_data(&unit);
    if (!check(dialog->findChild<QWidget*>("modelRow_Local_Reference_Frame") == nullptr,
               "Surface injections should not expose a local reference-frame selector"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = volume;
    dialog->refresh_from_unit_data(&unit);
    if (!check(dialog->findChild<QWidget*>("modelRow_Local_Reference_Frame") == nullptr,
               "Volume injections should not expose a local reference-frame selector"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = single;
    dialog->refresh_from_unit_data(&unit);
    if (!check(dialog->findChild<QWidget*>("modelRow_Local_Reference_Frame") != nullptr,
               "Point injections should expose a local reference-frame selector"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = plain_oriface_atomizer;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "Injector Inner Diameter") &&
                   has_field_row(dialog, "Orifice Length") &&
                   has_field_row(dialog, "Corner Radius of Curvature") &&
                   has_field_row(dialog, "Vapor Pressure") &&
                   !has_field_row(dialog, "X-Position") &&
                   !has_field_row(dialog, "Diameter") &&
                   !has_field_row(dialog, "Outer Diameter"),
               "Plain-orifice atomizers should expose their documented parameters"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = pressure_swirl_atomizer;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "Spray Half Angle") &&
                   has_field_row(dialog, "Upstream Pressure") &&
                   !has_field_row(dialog, "X-Position") &&
                   !has_field_row(dialog, "Outer Diameter"),
               "Pressure-swirl atomizers should expose spray and pressure parameters"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = flat_fan_atomizer;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "X-Fan Normal") &&
                   has_field_row(dialog, "Orifice Width") &&
                   has_field_row(dialog, "Flat Fan Sheet Constant") &&
                   !has_field_row(dialog, "X-Atomizer Axis"),
               "Flat-fan atomizers should use fan-normal rather than atomizer-axis inputs"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = air_blast_atomizer;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "Relative Velocity") &&
                   !has_field_row(dialog, "X-Position"),
               "Air-blast atomizers should hide particle initial-position inputs"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = effervescent_atomizer;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "Mixture Quality") &&
                   !has_field_row(dialog, "X-Position"),
               "Effervescent atomizers should hide particle initial-position inputs"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Massless;
    unit.inj.injector_data.injection_type = effervescent_atomizer;
    dialog->refresh_from_unit_data(&unit);
    if (!check(!has_field_row(dialog, "X-Atomizer Axis") &&
                   !has_field_row(dialog, "Mixture Quality"),
               "Massless effervescent injections should expose position only"))
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

    auto *injection_tabs = dialog->findChild<QTabWidget*>("tabWidget_injection");
    auto *parcel_tab = dialog->findChild<QWidget*>("tab_parcel");
    if (!check(injection_tabs != nullptr && parcel_tab != nullptr,
               "Injection tabs should expose the Parcel page"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.dpm_domain = "secondary-domain";
    unit.inj.injector_data.parcel_model = standard;
    unit.inj.injector_data.stochastic = true;
    unit.inj.injector_data.ntries = 7;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    const int parcel_tab_index = injection_tabs->indexOf(parcel_tab);
    if (!check(parcel_tab_index >= 0 &&
                   !injection_tabs->isTabEnabled(parcel_tab_index) &&
                   unit.inj.injector_data.parcel_model == const_diameter &&
                   unit.inj.injector_data.ntries == 1 &&
                   dialog->findChild<QWidget*>("modelRow_Eddy_Attempts") != nullptr &&
                   !dialog->findChild<QWidget*>("modelRow_Eddy_Attempts")->isEnabled(),
               "DDPM should disable Parcel settings and force constant diameter"))
    {
        delete parent;
        return 1;
    }
    auto *injection_type_editor = dialog->findChild<QUI_ComboBox*>("injectionTypeEditor");
    auto *injection_type_model = injection_type_editor != nullptr
        ? qobject_cast<QStandardItemModel *>(injection_type_editor->model())
        : nullptr;
    const int volume_item_index = injection_type_editor != nullptr
        ? injection_type_editor->findData(volume)
        : -1;
    if (!check(injection_type_model != nullptr && volume_item_index >= 0 &&
                   injection_type_model->item(volume_item_index)->isEnabled(),
               "DDPM should keep Volume available in the injection-type selector"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = volume;
    unit.inj.injector_data.volume_specification = zone;
    unit.inj.injector_data.tabulated_diam_dist = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.injection_type == volume &&
                   unit.inj.injector_data.tabulated_diam_dist &&
                   has_field_row(dialog, "Packing Limit") &&
                   !has_field_row(dialog, "Stream Specification") &&
                   !has_field_row(dialog, "Total Streams") &&
                   !has_field_row(dialog, "Streams Per Cell"),
               "DDPM volume injections should use packing limits, automatic starting points, and Tabulated distribution"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.dpm_domain = "none";
    unit.inj.injector_data.injection_type = volume;
    unit.inj.injector_data.volume_specification = bouning_geometry;
    unit.inj.injector_data.volume_bgeom_shapes = hexahedron;
    unit.inj.injector_data.volume_bgeom_min = QVector3D(4.0f, -2.0f, 8.0f);
    unit.inj.injector_data.volume_bgeom_max = QVector3D(-1.0f, 3.0f, 5.0f);
    unit.inj.injector_data.volume_bgeom_radius = -4.0;
    unit.inj.injector_data.volume_bgeom_viconeangle = 4.0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.volume_bgeom_min == QVector3D(-1.0f, -2.0f, 5.0f) &&
                   unit.inj.injector_data.volume_bgeom_max == QVector3D(4.0f, 3.0f, 8.0f) &&
                   unit.inj.injector_data.volume_bgeom_radius == 0.0 &&
                   unit.inj.injector_data.volume_bgeom_viconeangle ==
                       3.14159265358979323846,
               "Volume bounding geometry should normalize dimensions and angle"))
    {
        delete parent;
        return 1;
    }

    auto *diameter_distribution_editor = dialog->findChild<QUI_ComboBox*>(
        "diameterDistributionEditor");
    if (!check(diameter_distribution_editor != nullptr,
               "Diameter distribution editor should be available for volume injections"))
    {
        delete parent;
        return 1;
    }
    if (!check(diameter_distribution_editor->findText("tabulated") >= 0,
               "Volume injections should offer tabulated diameter distributions"))
    {
        delete parent;
        return 1;
    }

    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(!injection_tabs->isTabEnabled(parcel_tab_index) &&
                   unit.inj.injector_data.parcel_model == standard,
               "Volume injections should not expose the Parcel page"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = group;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(injection_tabs->isTabEnabled(parcel_tab_index),
               "Switching to a parcel-capable injection should re-enable the Parcel page"))
    {
        delete parent;
        return 1;
    }
    injection_type_editor = dialog->findChild<QUI_ComboBox*>("injectionTypeEditor");
    injection_type_model = injection_type_editor != nullptr
        ? qobject_cast<QStandardItemModel *>(injection_type_editor->model())
        : nullptr;
    const int restored_volume_index = injection_type_editor != nullptr
        ? injection_type_editor->findData(volume)
        : -1;
    if (!check(injection_type_model != nullptr && restored_volume_index >= 0 &&
                   injection_type_model->item(restored_volume_index)->isEnabled(),
               "Leaving DDPM should re-enable Volume in the injection-type selector"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.injection_type = volume;
    unit.inj.injector_data.parcel_model = standard;
    dialog->refresh_from_unit_data(&unit);
    auto *dpm_domain_editor = dialog->findChild<QUI_ComboBox*>(
        "discretePhaseDomainEditor");
    if (!check(dpm_domain_editor != nullptr,
               "DPM domain editor should be available"))
    {
        delete parent;
        return 1;
    }
    const int secondary_domain_index = dpm_domain_editor->findText("secondary-domain");
    dpm_domain_editor->setCurrentIndex(secondary_domain_index);
    application.processEvents();
    if (!check(unit.inj.injector_data.injection_type == volume &&
                   unit.inj.injector_data.parcel_model == const_diameter &&
                   !injection_tabs->isTabEnabled(parcel_tab_index),
               "Direct DDPM selection should keep Volume and normalize Parcel settings"))
    {
        delete parent;
        return 1;
    }
    dpm_domain_editor = dialog->findChild<QUI_ComboBox*>(
        "discretePhaseDomainEditor");
    dpm_domain_editor->setCurrentIndex(dpm_domain_editor->findText("none"));
    application.processEvents();
    if (!check(!injection_tabs->isTabEnabled(parcel_tab_index) &&
                   unit.inj.injector_data.parcel_model == standard,
               "Leaving DDPM should not expose Parcel for Volume injections"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.injection_type = group;
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(injection_tabs->isTabEnabled(parcel_tab_index),
               "Directly leaving DDPM should restore Parcel for Group injections"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Massless;
    unit.inj.injector_data.rotation_on = true;
    unit.inj.injector_data.rough_wall_on = true;
    unit.inj.injector_data.drag_law = nonspherical;
    unit.inj.injector_data.brownian_motion = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(!unit.inj.injector_data.rotation_on &&
                   !unit.inj.injector_data.rough_wall_on &&
                   unit.inj.injector_data.drag_law == spherical &&
                   !unit.inj.injector_data.brownian_motion,
               "Massless particles should clear inertial model state"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.seco_breakup_on = false;
    unit.inj.injector_data.seco_breakup_wave = false;
    unit.inj.injector_data.drag_law = dynamic_drag;
    dialog->refresh_from_unit_data(&unit);
    auto *drag_editor = dialog->findChild<QUI_ComboBox*>("modelEditor_Drag_Law");
    if (!check(drag_editor != nullptr &&
                   unit.inj.injector_data.drag_law == spherical &&
                   drag_editor->findText("Dynamic Drag") < 0,
               "Dynamic Drag should be unavailable without breakup"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.seco_breakup_on = true;
    unit.inj.injector_data.seco_breakup_wave = true;
    unit.inj.injector_data.drag_law = dynamic_drag;
    dialog->refresh_from_unit_data(&unit);
    drag_editor = dialog->findChild<QUI_ComboBox*>("modelEditor_Drag_Law");
    if (!check(drag_editor != nullptr &&
                   drag_editor->findText("Dynamic Drag") >= 0 &&
                   drag_editor->currentData().toInt() == dynamic_drag,
               "Dynamic Drag should be available with a selected breakup model"))
    {
        delete parent;
        return 1;
    }

    auto *rotation_row = dialog->findChild<QWidget*>("modelRow_Rotation");
    auto *rotation_editor = rotation_row != nullptr
        ? rotation_row->findChild<QUI_CheckBox*>()
        : nullptr;
    if (!check(rotation_editor != nullptr,
               "Rotation editor should be available for physical particles"))
    {
        delete parent;
        return 1;
    }
    rotation_editor->click();
    application.processEvents();
    if (!check(has_field_row(dialog, "X-Angular Velocity") &&
                   dialog->findChild<QWidget*>("modelRow_Rotational_Drag_Law") != nullptr,
               "Enabling rotation should expose rotational parameters"))
    {
        delete parent;
        return 1;
    }
    rotation_row = dialog->findChild<QWidget*>("modelRow_Rotation");
    rotation_editor = rotation_row != nullptr
        ? rotation_row->findChild<QUI_CheckBox*>()
        : nullptr;
    rotation_editor->click();
    application.processEvents();
    if (!check(!has_field_row(dialog, "X-Angular Velocity") &&
                   dialog->findChild<QWidget*>("modelRow_Rotational_Drag_Law") == nullptr,
               "Disabling rotation should hide rotational parameters"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = cone;
    unit.inj.injector_data.rotation_on = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(has_field_row(dialog, "Angular Velocity Magnitude") &&
                   !has_field_row(dialog, "X-Angular Velocity"),
               "Cone rotation should use angular-velocity magnitude"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = plain_oriface_atomizer;
    unit.inj.injector_data.rotation_on = true;
    unit.inj.injector_data.parcel_model = const_mass;
    dialog->refresh_from_unit_data(&unit);
    auto *atomizer_parcel_editor = dialog->findChild<QUI_ComboBox*>(
        "modelEditor_Parcel_Model");
    if (!check(!unit.inj.injector_data.rotation_on &&
                   unit.inj.injector_data.parcel_model == standard &&
                   atomizer_parcel_editor != nullptr &&
                   atomizer_parcel_editor->count() == 1 &&
                   !atomizer_parcel_editor->isEnabled() &&
                   !has_field_row(dialog, "Parcel Mass"),
               "Atomizer injections should lock rotation and parcel release to Standard"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = file_;
    unit.inj.injector_data.parcel_model = const_mass;
    dialog->refresh_from_unit_data(&unit);
    auto *file_parcel_editor = dialog->findChild<QUI_ComboBox*>(
        "modelEditor_Parcel_Model");
    if (!check(unit.inj.injector_data.parcel_model == standard &&
                   file_parcel_editor != nullptr &&
                   file_parcel_editor->count() == 1 &&
                   !file_parcel_editor->isEnabled(),
               "File injections should lock parcel release to Standard"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = group;
    dialog->refresh_from_unit_data(&unit);
    drag_editor = dialog->findChild<QUI_ComboBox*>("modelEditor_Drag_Law");
    if (drag_editor == nullptr)
    {
        delete parent;
        return 1;
    }
    const int stokes_index = drag_editor->findData(Strokes_Cunningham);
    drag_editor->setCurrentIndex(stokes_index);
    drag_editor->selection_committed();
    application.processEvents();
    if (!check(has_field_row(dialog, "Cunningham Correction") &&
                   !has_field_row(dialog, "Shape Factor"),
               "Changing drag law should rebuild its dependent parameters"))
    {
        delete parent;
        return 1;
    }

    auto *parcel_editor = dialog->findChild<QUI_ComboBox*>(
        "modelEditor_Parcel_Model");
    if (!check(parcel_editor != nullptr,
               "Parcel model editor should be available"))
    {
        delete parent;
        return 1;
    }
    const int constant_mass_index = parcel_editor->findText("Constant Mass");
    parcel_editor->setCurrentIndex(constant_mass_index);
    parcel_editor->selection_committed();
    application.processEvents();
    if (!check(has_field_row(dialog, "Parcel Mass") &&
                   !has_field_row(dialog, "Total Mass"),
               "Changing parcel model should rebuild its dependent parameters"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Combusting;
    unit.inj.injector_data.evaporating_liquid = false;
    unit.inj.injector_data.evaporating_species = "H2O";
    dialog->refresh_from_unit_data(&unit);
    if (!check(!unit.inj.injector_data.evaporating_liquid &&
                   unit.inj.injector_data.evaporating_species.isEmpty() &&
                   !evaporating_species_editor->isEnabled(),
               "Combusting particles should hide evaporating species without wet combustion"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.evaporating_liquid = true;
    unit.inj.injector_data.evaporating_species = "H2O";
    dialog->refresh_from_unit_data(&unit);
    if (!check(evaporating_species_editor->isEnabled(),
               "Wet combusting particles should enable evaporating species"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.drag_law = nonspherical;
    unit.inj.injector_data.shape_factor = 1.0;
    dialog->refresh_from_unit_data(&unit);
    auto *shape_factor_row = dialog->findChild<QWidget*>("modelRow_Shape_Factor");
    auto *shape_factor_editor = shape_factor_row != nullptr
        ? shape_factor_row->findChild<QUI_LineEdit*>()
        : nullptr;
    if (!check(shape_factor_editor != nullptr,
               "Nonspherical drag should expose the shape-factor editor"))
    {
        delete parent;
        return 1;
    }
    shape_factor_editor->setText("-1");
    if (!check(!shape_factor_editor->commit() &&
                   unit.inj.injector_data.shape_factor == 1.0,
               "Shape factor should reject values below zero"))
    {
        delete parent;
        return 1;
    }
    shape_factor_editor->setText("1/2");
    if (!check(shape_factor_editor->commit() &&
                   qFuzzyCompare(unit.inj.injector_data.shape_factor, 0.5),
               "Shape factor should retain expression input support"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = flat_fan_atomizer;
    unit.inj.injector_data.diameter = 4.0;
    unit.inj.injector_data.diameter2 = 1.0;
    unit.inj.injector_data.temperature = 600.0;
    unit.inj.injector_data.temperature2 = 300.0;
    unit.inj.injector_data.flow_rate = 2.0;
    unit.inj.injector_data.flow_rate2 = 0.5;
    unit.inj.injector_data.phi_start = 2.0;
    unit.inj.injector_data.phi_stop = -1.0;
    unit.inj.injector_data.atomizer_axis = QVector3D();
    unit.inj.injector_data.ff_normal = QVector3D();
    unit.inj.injector_data.atomizer_disp_angle = 200.0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.diameter == 1.0 &&
                   unit.inj.injector_data.diameter2 == 4.0 &&
                   unit.inj.injector_data.temperature == 300.0 &&
                   unit.inj.injector_data.temperature2 == 600.0 &&
                   unit.inj.injector_data.flow_rate == 0.5 &&
                   unit.inj.injector_data.flow_rate2 == 2.0 &&
                   unit.inj.injector_data.phi_start == -1.0 &&
                   unit.inj.injector_data.phi_stop == 2.0 &&
                   unit.inj.injector_data.atomizer_axis == QVector3D(1.0f, 0.0f, 0.0f) &&
                   unit.inj.injector_data.ff_normal == QVector3D(1.0f, 0.0f, 0.0f) &&
                   unit.inj.injector_data.atomizer_disp_angle < 180.0,
               "Injection ranges and atomizer directions should be normalized"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = air_blast_atomizer;
    unit.inj.injector_data.inner_diameter = 8.0;
    unit.inj.injector_data.outer_diameter = 2.0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.outer_diameter == 8.0,
               "Air-blast outer diameter should not be smaller than inner diameter"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = volume;
    unit.inj.injector_data.volume_fraction = 2.0;
    unit.inj.injector_data.volume_packing_limit_per_cell = -1.0;
    unit.inj.injector_data.time_scale_constant = std::numeric_limits<double>::quiet_NaN();
    unit.inj.injector_data.effer_quality = std::numeric_limits<double>::infinity();
    unit.inj.injector_data.half_angle = 10.0;
    unit.inj.injector_data.effer_half_angle_max = -1.0;
    unit.inj.injector_data.plain_const_a = -3.0;
    dialog->refresh_from_unit_data(&unit);
    if (!check(qFuzzyCompare(unit.inj.injector_data.volume_fraction, 1.0) &&
                   qFuzzyCompare(unit.inj.injector_data.volume_packing_limit_per_cell, 0.0) &&
                   qFuzzyCompare(unit.inj.injector_data.time_scale_constant, 0.0) &&
                   qFuzzyCompare(unit.inj.injector_data.effer_quality, 0.0) &&
                   unit.inj.injector_data.half_angle <= 1.5707963267948966 &&
                   qFuzzyCompare(unit.inj.injector_data.effer_half_angle_max, 0.0) &&
                   qFuzzyCompare(unit.inj.injector_data.plain_const_a, 0.0),
               "Exposed model scalars should normalize non-finite and out-of-range legacy values"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.injection_type = cone;
    unit.inj.injector_data.cone_type = ring;
    unit.inj.injector_data.radius = std::numeric_limits<double>::quiet_NaN();
    unit.inj.injector_data.inner_radius = std::numeric_limits<double>::infinity();
    unit.inj.injector_data.swirl_frac = std::numeric_limits<double>::quiet_NaN();
    unit.inj.injector_data.rr_min = std::numeric_limits<double>::quiet_NaN();
    unit.inj.injector_data.rr_max = std::numeric_limits<double>::infinity();
    unit.inj.injector_data.rr_mean = std::numeric_limits<double>::quiet_NaN();
    unit.inj.injector_data.rr_spread = std::numeric_limits<double>::quiet_NaN();
    unit.inj.injector_data.cloud_min_dia = std::numeric_limits<double>::quiet_NaN();
    unit.inj.injector_data.cloud_max_dia = std::numeric_limits<double>::infinity();
    unit.inj.injector_data.stagger_radius = std::numeric_limits<double>::quiet_NaN();
    unit.inj.injector_data.shape_factor = std::numeric_limits<double>::quiet_NaN();
    dialog->refresh_from_unit_data(&unit);
    if (!check(std::isfinite(unit.inj.injector_data.radius) &&
                   std::isfinite(unit.inj.injector_data.inner_radius) &&
                   unit.inj.injector_data.radius > 0.0 &&
                   unit.inj.injector_data.inner_radius <= 0.95 * unit.inj.injector_data.radius &&
                   unit.inj.injector_data.swirl_frac == 0.0 &&
                   unit.inj.injector_data.rr_min == 0.0 &&
                   unit.inj.injector_data.rr_max == 0.0 &&
                   unit.inj.injector_data.rr_mean == 0.0 &&
                   unit.inj.injector_data.rr_spread > 0.0 &&
                   unit.inj.injector_data.cloud_min_dia == 0.0 &&
                   unit.inj.injector_data.cloud_max_dia == 0.0 &&
                   unit.inj.injector_data.stagger_radius == 0.0 &&
                   unit.inj.injector_data.shape_factor == 0.0,
               "Remaining model scalars should normalize non-finite legacy values"))
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

    unit.inj.injector_data.type = Combusting;
    unit.inj.injector_data.evaporating_liquid = true;
    unit.inj.injector_data.evaporating_material = "water";
    dialog->refresh_from_unit_data(&unit);
    auto *wet_combustion_tab = dialog->findChild<QWidget*>("tab_wet_combustion");
    const int wet_tab_index = wet_combustion_tab != nullptr
        ? injection_tabs->indexOf(wet_combustion_tab)
        : -1;
    if (!check(wet_tab_index >= 0 && injection_tabs->isTabVisible(wet_tab_index),
               "Wet Combustion should be visible for combusting particles"))
    {
        delete parent;
        return 1;
    }
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

    Unit_Edit_Case_Context restricted_context;
    restricted_context.energy_equation = Unit_Edit_Feature_State::Disabled;
    restricted_context.unsteady_particle_tracking = Unit_Edit_Feature_State::Disabled;
    restricted_context.three_dimensional = Unit_Edit_Feature_State::Disabled;
    restricted_context.dem = Unit_Edit_Feature_State::Enabled;
    restricted_context.reflect_boundary = Unit_Edit_Feature_State::Disabled;
    restricted_context.multiple_surface_reaction = Unit_Edit_Feature_State::Disabled;
    unit.inj.injector_data.type = Combusting;
    unit.inj.injector_data.injection_type = cone;
    unit.inj.injector_data.drag_law = dynamic_drag;
    unit.inj.injector_data.brownian_motion = true;
    unit.inj.injector_data.rough_wall_on = true;
    unit.inj.injector_data.oxidizing_species = "O2";
    unit.inj.injector_data.product_species = "CO2";
    dialog->set_case_context(restricted_context);
    application.processEvents();
    injection_type_editor = dialog->findChild<QUI_ComboBox*>("injectionTypeEditor");
    injection_type_model = injection_type_editor != nullptr
        ? qobject_cast<QStandardItemModel *>(injection_type_editor->model())
        : nullptr;
    const int cone_item_index = injection_type_editor != nullptr
        ? injection_type_editor->findData(cone)
        : -1;
    const int flat_fan_item_index = injection_type_editor != nullptr
        ? injection_type_editor->findData(flat_fan_atomizer)
        : -1;
    if (!check(unit.inj.injector_data.injection_type == single &&
                   unit.inj.injector_data.drag_law == spherical &&
                   !unit.inj.injector_data.brownian_motion &&
                   !unit.inj.injector_data.rough_wall_on &&
                   unit.inj.injector_data.oxidizing_species.isEmpty() &&
                   unit.inj.injector_data.product_species.isEmpty() &&
                   injection_type_model != nullptr && cone_item_index >= 0 &&
                   flat_fan_item_index >= 0 &&
                   !injection_type_model->item(cone_item_index)->isEnabled() &&
                   !injection_type_model->item(flat_fan_item_index)->isEnabled() &&
                   dialog->findChild<QWidget*>("modelRow_Rough_Wall") != nullptr &&
                   !dialog->findChild<QWidget*>("modelRow_Rough_Wall")->isEnabled() &&
                   dialog->findChild<QWidget*>("modelRow_Brownian_Motion") != nullptr &&
                   !dialog->findChild<QWidget*>("modelRow_Brownian_Motion")->isEnabled() &&
                   has_field_row(dialog, "X-Position") &&
                   !has_field_row(dialog, "Z-Position") &&
                   !injection_tabs->isTabEnabled(parcel_tab_index) &&
                   dialog->findChild<QUI_ComboBox*>("modelEditor_Drag_Law") != nullptr &&
                   dialog->findChild<QUI_ComboBox*>("modelEditor_Drag_Law")->findData(dynamic_drag) < 0,
               "Known case constraints should lock unsupported injection and physical models"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = group;
    for (const DPM_Type heat_dependent_type : {Droplet, Combusting, Multicomponent})
    {
        unit.inj.injector_data.type = heat_dependent_type;
        dialog->refresh_from_unit_data(&unit);
        application.processEvents();
        bool droplet_enabled = true;
        bool combusting_enabled = true;
        bool multicomponent_enabled = true;
        bool inert_enabled = false;
        for (QRadioButton *button : dialog->findChildren<QRadioButton *>())
        {
            if (button == nullptr)
            {
                continue;
            }
            if (button->text() == "Droplet") droplet_enabled = button->isEnabled();
            if (button->text() == "Combusting") combusting_enabled = button->isEnabled();
            if (button->text() == "Multicomponent") multicomponent_enabled = button->isEnabled();
            if (button->text() == "Inert") inert_enabled = button->isEnabled();
        }
        if (!check(unit.inj.injector_data.type == Inert &&
                       !droplet_enabled && !combusting_enabled &&
                       !multicomponent_enabled && inert_enabled,
                   "Heat-dependent particle types should be unavailable when the energy equation is disabled"))
        {
            delete parent;
            return 1;
        }
    }

    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.injection_type = condensate;
    dialog->refresh_from_unit_data(&unit);
    if (!check(unit.inj.injector_data.injection_type == single &&
                   unit.inj.injector_data.type == Inert,
               "Condensate should fall back when the energy equation is disabled"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.injection_type = group;
    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.rotation_on = true;
    dialog->refresh_from_unit_data(&unit);
    if (!check(!has_field_row(dialog, "X-Angular Velocity") &&
                   has_field_row(dialog, "Z-Angular Velocity"),
               "Two-dimensional rotation should expose only normal angular velocity"))
    {
        delete parent;
        return 1;
    }

    Unit_Edit_Case_Context two_dimensional_context;
    two_dimensional_context.three_dimensional = Unit_Edit_Feature_State::Disabled;
    unit.inj.injector_data.type = Inert;
    unit.inj.injector_data.injection_type = pressure_swirl_atomizer;
    dialog->set_case_context(two_dimensional_context);
    application.processEvents();
    if (!check(!has_field_row(dialog, "X-Atomizer Axis") &&
                   !has_field_row(dialog, "Y-Atomizer Axis") &&
                   !has_field_row(dialog, "Z-Atomizer Axis"),
               "Two-dimensional atomizer injections should hide axis inputs"))
    {
        delete parent;
        return 1;
    }
    dialog->set_case_context(Unit_Edit_Case_Context{});

    unit.inj.injector_data.type = Droplet;
    dialog->set_case_context(Unit_Edit_Case_Context{});
    application.processEvents();
    bool unknown_droplet_enabled = false;
    for (QRadioButton *button : dialog->findChildren<QRadioButton *>())
    {
        if (button != nullptr && button->text() == "Droplet")
        {
            unknown_droplet_enabled = button->isEnabled();
        }
    }
    if (!check(unknown_droplet_enabled,
               "Unknown energy-equation context should keep Droplet selectable"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.type = static_cast<DPM_Type>(999);
    unit.inj.injector_data.injection_type = static_cast<Injection_Type>(999);
    unit.inj.injector_data.cone_type = static_cast<Cone_Type>(999);
    unit.inj.injector_data.parcel_model = static_cast<Parcel_Model>(999);
    unit.inj.injector_data.drag_law = static_cast<Drag_Law>(999);
    unit.inj.injector_data.volume_specification = static_cast<Volume_Specification>(999);
    unit.inj.injector_data.volume_streams_spec = static_cast<Volume_Streams_Spec>(999);
    unit.inj.injector_data.volume_bgeom_shapes = static_cast<Volume_Bgeom_Shapes>(999);
    unit.inj.injector_data.rot_drag_law = static_cast<Rot_Drag_Law>(999);
    unit.inj.injector_data.rot_lift_law = static_cast<Rot_Lift_Law>(999);
    unit.inj.injector_data.pos = QVector3D(std::numeric_limits<float>::quiet_NaN(), 1.0f, 2.0f);
    unit.inj.injector_data.vel = QVector3D(1.0f, std::numeric_limits<float>::infinity(), -2.0f);
    dialog->refresh_from_unit_data(&unit);
    application.processEvents();
    if (!check(unit.inj.injector_data.type == Droplet &&
                   unit.inj.injector_data.injection_type == single &&
                   unit.inj.injector_data.cone_type == point &&
                   unit.inj.injector_data.parcel_model == standard &&
                   unit.inj.injector_data.drag_law == spherical &&
                   unit.inj.injector_data.volume_specification == zone &&
                   unit.inj.injector_data.volume_streams_spec == total_parcel_count &&
                   unit.inj.injector_data.volume_bgeom_shapes == sphere &&
                   unit.inj.injector_data.rot_drag_law == none &&
                   unit.inj.injector_data.rot_lift_law == none_ &&
                   std::isfinite(unit.inj.injector_data.pos.x()) &&
                   std::isfinite(unit.inj.injector_data.vel.y()),
               "Invalid enum and vector values should be normalized before UI rebuild"))
    {
        delete parent;
        return 1;
    }

    Unit_Edit_Case_Context steady_context;
    steady_context.unsteady_particle_tracking = Unit_Edit_Feature_State::Disabled;
    unit.inj.injector_data.injection_type = group;
    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.seco_breakup_on = true;
    unit.inj.injector_data.seco_breakup_wave = true;
    unit.inj.injector_data.drag_law = dynamic_drag;
    unit.inj.injector_data.parcel_model = const_mass;
    dialog->set_case_context(steady_context);
    application.processEvents();
    const int steady_parcel_index = injection_tabs->indexOf(parcel_tab);
    if (!check(unit.inj.injector_data.parcel_model == standard &&
                   unit.inj.injector_data.drag_law == spherical &&
                   steady_parcel_index >= 0 &&
                   !injection_tabs->isTabEnabled(steady_parcel_index),
               "Steady tracking should reset parcel release and dynamic drag"))
    {
        delete parent;
        return 1;
    }

    Unit_Edit_Case_Context unsteady_context;
    unsteady_context.unsteady_particle_tracking = Unit_Edit_Feature_State::Enabled;
    unit.inj.injector_data.type = Inert;
    unit.inj.injector_data.injection_type = group;
    unit.inj.injector_data.stochastic = true;
    unit.inj.injector_data.ntries = 8;
    dialog->set_case_context(unsteady_context);
    application.processEvents();
    auto *eddy_attempts_row = dialog->findChild<QWidget*>("modelRow_Eddy_Attempts");
    if (!check(unit.inj.injector_data.ntries == 1 &&
                   eddy_attempts_row != nullptr && !eddy_attempts_row->isEnabled(),
               "Unsteady stochastic tracking should force one Eddy attempt"))
    {
        delete parent;
        return 1;
    }

    auto *random_eddy_row = dialog->findChild<QWidget*>("modelRow_Random_Eddy");
    auto *random_eddy_editor = random_eddy_row != nullptr
        ? random_eddy_row->findChild<QUI_CheckBox*>()
        : nullptr;
    time_scale_row = dialog->findChild<QWidget*>("modelRow_Time_Scale_Constant");
    if (!check(random_eddy_editor != nullptr &&
                   time_scale_row != nullptr &&
                   !time_scale_row->isEnabled(),
               "Random Eddy should initially lock its time-scale input"))
    {
        delete parent;
        return 1;
    }
    random_eddy_editor->click();
    application.processEvents();
    time_scale_row = dialog->findChild<QWidget*>("modelRow_Time_Scale_Constant");
    if (!check(unit.inj.injector_data.random_eddy &&
                   time_scale_row != nullptr &&
                   time_scale_row->isEnabled(),
               "Enabling Random Eddy should unlock its time-scale input"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.random_eddy = false;
    dialog->refresh_from_unit_data(&unit);
    time_scale_row = dialog->findChild<QWidget*>("modelRow_Time_Scale_Constant");
    if (!check(!unit.inj.injector_data.random_eddy &&
                   time_scale_row != nullptr &&
                   !time_scale_row->isEnabled(),
               "External Random Eddy changes should relock its time-scale input"))
    {
        delete parent;
        return 1;
    }

    unit.inj.injector_data.stochastic = false;
    dialog->set_case_context(Unit_Edit_Case_Context{});
    application.processEvents();

    Unit_Edit_Case_Context chemistry_limited_context;
    chemistry_limited_context.active_chemistry_species_count = 1;
    chemistry_limited_context.nonpremixed_combustion = Unit_Edit_Feature_State::Disabled;
    unit.inj.injector_data.type = Droplet;
    unit.inj.injector_data.injection_type = group;
    dialog->set_case_context(chemistry_limited_context);
    application.processEvents();
    if (!check(unit.inj.injector_data.type == Inert,
               "Droplet should fall back when chemistry has fewer than two active species"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.type = Droplet;
    chemistry_limited_context.nonpremixed_combustion = Unit_Edit_Feature_State::Enabled;
    dialog->set_case_context(chemistry_limited_context);
    application.processEvents();
    bool chemistry_enabled_droplet = false;
    for (QRadioButton *button : dialog->findChildren<QRadioButton *>())
    {
        if (button != nullptr && button->text() == "Droplet")
        {
            chemistry_enabled_droplet = button->isEnabled();
        }
    }
    if (!check(chemistry_enabled_droplet,
               "Non-premixed combustion should allow Droplet with one active species"))
    {
        delete parent;
        return 1;
    }

    chemistry_limited_context.nonpremixed_combustion =
        Unit_Edit_Feature_State::Disabled;
    chemistry_limited_context.active_chemistry_species_count = 1;
    unit.inj.injector_data.type = Combusting;
    dialog->set_case_context(chemistry_limited_context);
    application.processEvents();
    if (!check(unit.inj.injector_data.type == Inert,
               "Combusting should require two active chemistry species"))
    {
        delete parent;
        return 1;
    }

    chemistry_limited_context.active_chemistry_species_count = 2;
    unit.inj.injector_data.type = Combusting;
    dialog->set_case_context(chemistry_limited_context);
    application.processEvents();
    bool chemistry_enabled_combusting = false;
    for (QRadioButton *button : dialog->findChildren<QRadioButton *>())
    {
        if (button != nullptr && button->text() == "Combusting")
        {
            chemistry_enabled_combusting = button->isEnabled();
        }
    }
    if (!check(chemistry_enabled_combusting,
               "Combusting should be available with two active chemistry species"))
    {
        delete parent;
        return 1;
    }

    Unit_Edit_Case_Context material_reaction_context;
    material_reaction_context.material_multiple_surface_reaction =
        Unit_Edit_Feature_State::Enabled;
    unit.inj.injector_data.type = Combusting;
    unit.inj.injector_data.injection_type = group;
    unit.inj.injector_data.oxidizing_species = "O2";
    unit.inj.injector_data.product_species = "CO2";
    dialog->set_case_context(material_reaction_context);
    application.processEvents();
    if (!check(unit.inj.injector_data.oxidizing_species.isEmpty() &&
                   unit.inj.injector_data.product_species.isEmpty() &&
                   !oxidizing_species_editor->isEnabled() &&
                   !product_species_editor->isEnabled(),
               "Multiple-surface-reaction materials should own reaction species"))
    {
        delete parent;
        return 1;
    }
    unit.inj.injector_data.oxidizing_species = "O2";
    unit.inj.injector_data.product_species = "CO2";
    material_reaction_context.material_multiple_surface_reaction =
        Unit_Edit_Feature_State::Unknown;
    dialog->set_case_context(material_reaction_context);
    application.processEvents();
    if (!check(oxidizing_species_editor->isEnabled() &&
                   product_species_editor->isEnabled(),
               "Unknown material reaction metadata should keep reaction species editable"))
    {
        delete parent;
        return 1;
    }
    dialog->set_case_context(Unit_Edit_Case_Context{});
    application.processEvents();

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
