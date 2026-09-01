#include "qUI_components.h"

#include <QFont>
#include <QSizePolicy>
#include <QLocale>
#include <QLayoutItem>
#include <QSignalBlocker>

#include "unit_system.h"
#include <tinyexpr.h>

#include <array>
#include <cmath>
#include <memory>
#include <limits>

namespace
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kDegToRad = kPi / 180.0;
constexpr double kIntegerTolerance = 1.0e-9;

QString normalize_expression_text(QString text)
{
    text.replace(QChar(0xFF08), '('); // （
    text.replace(QChar(0xFF09), ')'); // ）
    text.replace(QChar(0xFF0B), '+'); // ＋
    text.replace(QChar(0xFF0D), '-'); // －
    text.replace(QChar(0x2212), '-'); // −
    text.replace(QChar(0x00D7), '*'); // ×
    text.replace(QChar(0x00F7), '/'); // ÷
    text.replace(QChar(0xFF0C), ','); // ，
    text.replace(QChar(0x3001), ','); // 、
    text.replace(QChar(0x03C0), "pi"); // π
    return text.trimmed();
}

double degree_sin(double degrees)
{
    return std::sin(degrees * kDegToRad);
}

double degree_cos(double degrees)
{
    return std::cos(degrees * kDegToRad);
}

double degree_tan(double degrees)
{
    return std::tan(degrees * kDegToRad);
}

double degree_asin(double value)
{
    return std::asin(value) / kDegToRad;
}

double degree_acos(double value)
{
    return std::acos(value) / kDegToRad;
}

double degree_atan(double value)
{
    return std::atan(value) / kDegToRad;
}

double natural_log(double value)
{
    return std::log(value);
}

double decimal_log(double value)
{
    return std::log10(value);
}

double power_value(double base, double exponent)
{
    return std::pow(base, exponent);
}

double minimum_value(double lhs, double rhs)
{
    return lhs < rhs ? lhs : rhs;
}

double maximum_value(double lhs, double rhs)
{
    return lhs > rhs ? lhs : rhs;
}

struct TinyExpr_Deleter
{
    void operator()(te_expr *expression) const
    {
        te_free(expression);
    }
};

QString make_parse_error_message(const QString &expression, int error_position)
{
    if (error_position <= 0)
    {
        return "Invalid expression.";
    }

    const qsizetype index = static_cast<qsizetype>(error_position - 1);
    const QString fragment = expression.mid(index, 12);
    if (fragment.isEmpty())
    {
        return "Invalid expression near the end of input.";
    }

    return QString("Invalid expression near '%1'.").arg(fragment);
}

bool evaluate_expression(const QString &source_text, double &value, QString &error_message)
{
    const QString expression = normalize_expression_text(source_text);
    if (expression.isEmpty())
    {
        error_message = "Expression is empty.";
        return false;
    }

    const QByteArray expression_utf8 = expression.toUtf8();
    const double pi_value = kPi;
    const double e_value = std::exp(1.0);

    const std::array<te_variable, 13> variables = {{
        {"pi", &pi_value, TE_VARIABLE, nullptr},
        {"e", &e_value, TE_VARIABLE, nullptr},
        {"sin", reinterpret_cast<const void *>(degree_sin), TE_FUNCTION1 | TE_FLAG_PURE, nullptr},
        {"cos", reinterpret_cast<const void *>(degree_cos), TE_FUNCTION1 | TE_FLAG_PURE, nullptr},
        {"tan", reinterpret_cast<const void *>(degree_tan), TE_FUNCTION1 | TE_FLAG_PURE, nullptr},
        {"asin", reinterpret_cast<const void *>(degree_asin), TE_FUNCTION1 | TE_FLAG_PURE, nullptr},
        {"acos", reinterpret_cast<const void *>(degree_acos), TE_FUNCTION1 | TE_FLAG_PURE, nullptr},
        {"atan", reinterpret_cast<const void *>(degree_atan), TE_FUNCTION1 | TE_FLAG_PURE, nullptr},
        {"ln", reinterpret_cast<const void *>(natural_log), TE_FUNCTION1 | TE_FLAG_PURE, nullptr},
        {"log", reinterpret_cast<const void *>(decimal_log), TE_FUNCTION1 | TE_FLAG_PURE, nullptr},
        {"pow", reinterpret_cast<const void *>(power_value), TE_FUNCTION2 | TE_FLAG_PURE, nullptr},
        {"min", reinterpret_cast<const void *>(minimum_value), TE_FUNCTION2 | TE_FLAG_PURE, nullptr},
        {"max", reinterpret_cast<const void *>(maximum_value), TE_FUNCTION2 | TE_FLAG_PURE, nullptr},
    }};

    int error_position = 0;
    std::unique_ptr<te_expr, TinyExpr_Deleter> compiled_expression(
        te_compile(expression_utf8.constData(), variables.data(), static_cast<int>(variables.size()), &error_position));
    if (!compiled_expression)
    {
        error_message = make_parse_error_message(expression, error_position);
        return false;
    }

    value = te_eval(compiled_expression.get());
    if (!std::isfinite(value))
    {
        error_message = "Expression result is not finite.";
        return false;
    }

    return true;
}

QString format_double_value(double value)
{
    return QLocale::c().toString(value, 'g', 15);
}

QString line_edit_style_sheet(bool invalid)
{
    if (invalid)
    {
        return QString(
            "QLineEdit {"
            "  border: 1px solid rgb(214, 92, 92);"
            "  border-radius: 6px;"
            "  background: rgb(63, 36, 36);"
            "  color: rgb(245, 245, 245);"
            "  selection-background-color: rgb(120, 156, 214);"
            "  selection-color: white;"
            "  padding: 4px 8px;"
            "}");
    }

    return QString(
        "QLineEdit {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 6px;"
        "  background: rgb(39, 39, 39);"
        "  color: rgb(241, 241, 241);"
        "  selection-background-color: rgb(120, 156, 214);"
        "  selection-color: white;"
        "  padding: 4px 8px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid rgb(118, 168, 255);"
        "}"
        "QLineEdit:disabled {"
        "  border: 1px solid rgb(68, 68, 68);"
        "  background: rgb(31, 31, 31);"
        "  color: rgb(130, 130, 130);"
        "}");
}

QString combo_box_style_sheet()
{
    return QString(
        "QComboBox {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 6px;"
        "  background: rgb(39, 39, 39);"
        "  color: rgb(241, 241, 241);"
        "  selection-background-color: rgb(120, 156, 214);"
        "  selection-color: white;"
        "  padding: 4px 30px 4px 10px;"
        "}"
        "QComboBox:focus {"
        "  border: 1px solid rgb(118, 168, 255);"
        "}"
        "QComboBox:disabled {"
        "  border: 1px solid rgb(68, 68, 68);"
        "  background: rgb(31, 31, 31);"
        "  color: rgb(130, 130, 130);"
        "}"
        "QComboBox::drop-down {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  margin: 1px 1px 1px 0px;"
        "  border: none;"
        "  border-left: 1px solid rgb(205, 211, 220);"
        "  border-top-right-radius: 5px;"
        "  border-bottom-right-radius: 5px;"
        "  width: 24px;"
        "  background: rgb(54, 54, 54);"
        "}"
        "QComboBox::down-arrow {"
        "  image: url(:/ui/icons/chevron-down.svg);"
        "  width: 12px;"
        "  height: 12px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 6px;"
        "  background: rgb(39, 39, 39);"
        "  color: rgb(241, 241, 241);"
        "  padding: 4px 0px;"
        "  outline: 0;"
        "  selection-background-color: rgb(68, 103, 167);"
        "  selection-color: white;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "  min-height: 24px;"
        "  padding: 4px 10px;"
        "}");
}

QString scroll_bar_style_sheet()
{
    return QString(
        "QScrollBar:vertical {"
        "  background: rgb(30, 30, 30);"
        "  width: 12px;"
        "  margin: 12px 0px 12px 0px;"
        "  border: none;"
        "  border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: rgb(205, 211, 220);"
        "  min-height: 28px;"
        "  border-radius: 6px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "  background: rgb(221, 226, 233);"
        "}"
        "QScrollBar::handle:vertical:pressed {"
        "  background: rgb(236, 240, 246);"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "  background: rgb(54, 54, 54);"
        "  height: 12px;"
        "  border: none;"
        "}"
        "QScrollBar::sub-line:vertical {"
        "  border-top-left-radius: 6px;"
        "  border-top-right-radius: 6px;"
        "}"
        "QScrollBar::add-line:vertical {"
        "  border-bottom-left-radius: 6px;"
        "  border-bottom-right-radius: 6px;"
        "}"
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
        "  background: transparent;"
        "}"
        "QScrollBar:horizontal {"
        "  background: rgb(30, 30, 30);"
        "  height: 12px;"
        "  margin: 0px 12px 0px 12px;"
        "  border: none;"
        "  border-radius: 6px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "  background: rgb(205, 211, 220);"
        "  min-width: 28px;"
        "  border-radius: 6px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "  background: rgb(221, 226, 233);"
        "}"
        "QScrollBar::handle:horizontal:pressed {"
        "  background: rgb(236, 240, 246);"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "  background: rgb(54, 54, 54);"
        "  width: 12px;"
        "  border: none;"
        "}"
        "QScrollBar::sub-line:horizontal {"
        "  border-top-left-radius: 6px;"
        "  border-bottom-left-radius: 6px;"
        "}"
        "QScrollBar::add-line:horizontal {"
        "  border-top-right-radius: 6px;"
        "  border-bottom-right-radius: 6px;"
        "}"
        "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
        "  background: transparent;"
        "}");
}

QString spin_box_style_sheet()
{
    return QString(
        "QSpinBox {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 6px;"
        "  background: rgb(39, 39, 39);"
        "  color: rgb(241, 241, 241);"
        "  selection-background-color: rgb(120, 156, 214);"
        "  selection-color: white;"
        "  padding: 4px 30px 4px 8px;"
        "}"
        "QSpinBox:focus {"
        "  border: 1px solid rgb(118, 168, 255);"
        "}"
        "QSpinBox:disabled {"
        "  border: 1px solid rgb(68, 68, 68);"
        "  background: rgb(31, 31, 31);"
        "  color: rgb(130, 130, 130);"
        "}"
        "QSpinBox::up-button {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: top right;"
        "  width: 24px;"
        "  height: 13px;"
        "  margin: 1px 1px 0px 0px;"
        "  border-left: 1px solid rgb(205, 211, 220);"
        "  border-bottom: 1px solid rgb(205, 211, 220);"
        "  border-top-right-radius: 5px;"
        "  background: rgb(54, 54, 54);"
        "}"
        "QSpinBox::down-button {"
        "  subcontrol-origin: padding;"
        "  subcontrol-position: bottom right;"
        "  width: 24px;"
        "  height: 13px;"
        "  margin: 0px 1px 1px 0px;"
        "  border-left: 1px solid rgb(205, 211, 220);"
        "  border-bottom-right-radius: 5px;"
        "  background: rgb(54, 54, 54);"
        "}"
        "QSpinBox::up-button:hover,"
        "QSpinBox::down-button:hover {"
        "  background: rgb(58, 58, 58);"
        "}"
        "QSpinBox::up-button:pressed,"
        "QSpinBox::down-button:pressed {"
        "  background: rgb(68, 68, 68);"
        "}"
        "QSpinBox::up-arrow {"
        "  image: url(:/ui/icons/chevron-up.svg);"
        "  width: 10px;"
        "  height: 10px;"
        "}"
        "QSpinBox::down-arrow {"
        "  image: url(:/ui/icons/chevron-down.svg);"
        "  width: 10px;"
        "  height: 10px;"
        "}");
}

QString check_box_style_sheet()
{
    return QString(
        "QCheckBox {"
        "  spacing: 6px;"
        "}");
}

QString push_button_style_sheet(bool accent_mode)
{
    if (accent_mode)
    {
        return QString(
            "QPushButton {"
            "  border: 1px solid rgb(74, 125, 232);"
            "  border-radius: 6px;"
            "  background: rgb(74, 125, 232);"
            "  color: white;"
            "  padding: 6px 14px;"
            "}"
            "QPushButton:hover {"
            "  background: rgb(62, 111, 216);"
            "}");
    }

    return QString(
        "QPushButton {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 6px;"
        "  background: white;"
        "  padding: 6px 14px;"
        "}"
        "QPushButton:hover {"
        "  border: 1px solid rgb(120, 156, 214);"
        "}");
}

QString radio_group_panel_style_sheet()
{
    return QString(
        "QUI_RadioGroup {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 8px;"
        "  background: rgb(29, 29, 29);"
        "}"
        "QUI_RadioGroup QRadioButton {"
        "  background: transparent;"
        "  color: rgb(241, 241, 241);"
        "  padding: 0px;"
        "  margin: 0px;"
        "}"
        "QUI_RadioGroup QLabel {"
        "  background: transparent;"
        "  color: rgb(241, 241, 241);"
        "}");
}

QString group_box_style_sheet_impl(bool reserve_title_space)
{
    const QFont title_font = QFont();
    const QString title_font_family = title_font.family();
    const QString title_font_size = QLocale::c().toString(title_font.pointSizeF(), 'f', 1);
    const QString title_margin_top = reserve_title_space ? "24px" : "0px";
    const QString title_padding_top = reserve_title_space ? "6px" : "3px";
    const QString title_rules = reserve_title_space
        ? QString(
              "QGroupBox::title {"
              "  subcontrol-origin: margin;"
              "  subcontrol-position: top left;"
              "  left: 12px;"
              "  top: 0px;"
              "  padding: 0px;"
              "  color: rgb(241, 241, 241);"
              "  background: transparent;"
              "  font-family: \"%1\";"
              "  font-size: %2pt;"
              "  font-weight: 400;"
              "}")
              .arg(title_font_family, title_font_size)
        : QString();

    return QString(
        "QGroupBox {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 8px;"
        "  background: rgb(29, 29, 29);"
        "  color: rgb(241, 241, 241);"
        "  margin-top: %1;"
        "  padding-top: %2;"
        "}"
        "QGroupBox QLabel,"
        "QGroupBox QCheckBox,"
        "QGroupBox QRadioButton {"
        "  background: transparent;"
        "  color: rgb(241, 241, 241);"
        "}%3").arg(title_margin_top, title_padding_top, title_rules);
}

QString group_box_style_sheet()
{
    return group_box_style_sheet_impl(true);
}
}

QString qui_group_box_style_sheet()
{
    return group_box_style_sheet();
}

QString qui_group_box_body_style_sheet()
{
    return group_box_style_sheet_impl(false);
}

QString qui_tab_widget_style_sheet()
{
    return QString(
        "QTabBar {"
        "  qproperty-expanding: true;"
        "}"
        "QTabWidget::tab-bar {"
        "  left: 8px;"
        "}"
        "QTabWidget::pane {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 8px;"
        "  background: rgb(33, 33, 33);"
        "  top: -1px;"
        "}"
        "QTabBar::tab {"
        "  background: rgb(48, 48, 48);"
        "  color: rgb(215, 219, 226);"
        "  border: 1px solid rgb(126, 134, 146);"
        "  border-bottom: none;"
        "  border-top-left-radius: 8px;"
        "  border-top-right-radius: 8px;"
        "  padding: 8px 14px;"
        "  margin-right: 4px;"
        "  margin-top: 3px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: rgb(33, 33, 33);"
        "  color: rgb(241, 241, 241);"
        "  border-color: rgb(205, 211, 220);"
        "  border-bottom-color: rgb(33, 33, 33);"
        "  margin-top: 0px;"
        "  margin-bottom: -1px;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background: rgb(58, 58, 58);"
        "  color: rgb(241, 241, 241);"
        "}"
        "QTabBar::tab:disabled {"
        "  color: rgb(130, 130, 130);"
        "  background: rgb(35, 35, 35);"
        "  border-color: rgb(78, 78, 78);"
        "}");
}

QString qui_scroll_area_style_sheet()
{
    return QString(
        "QScrollArea, QAbstractScrollArea {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 0px;"
        "  background: rgb(29, 29, 29);"
        "}"
        "QScrollArea::viewport {"
        "  background: rgb(29, 29, 29);"
        "  border-radius: 0px;"
        "}"
        "QScrollArea > QWidget > QWidget {"
        "  background: rgb(29, 29, 29);"
        "}"
        "QAbstractScrollArea::corner {"
        "  background: rgb(29, 29, 29);"
        "}"
        + scroll_bar_style_sheet());
}

QString qui_table_widget_style_sheet()
{
    return QString(
        "QTableWidget {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 8px;"
        "  background: rgb(33, 33, 33);"
        "  alternate-background-color: rgb(39, 39, 39);"
        "  color: rgb(241, 241, 241);"
        "  gridline-color: rgb(70, 70, 70);"
        "  selection-background-color: rgb(74, 108, 172);"
        "  selection-color: white;"
        "}"
        "QTableWidget::item {"
        "  padding: 4px 6px;"
        "}"
        "QTableWidget::item:selected {"
        "  color: white;"
        "}"
        "QHeaderView::section {"
        "  background: rgb(48, 48, 48);"
        "  color: rgb(241, 241, 241);"
        "  border: 1px solid rgb(88, 94, 104);"
        "  padding: 6px 8px;"
        "  font-weight: 600;"
        "}"
        "QTableCornerButton::section {"
        "  background: rgb(48, 48, 48);"
        "  border: 1px solid rgb(88, 94, 104);"
        "}"
        + scroll_bar_style_sheet());
}

QUI_Label::QUI_Label(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    setMinimumWidth(160);
    setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QFont label_font = font();
    label_font.setPointSizeF(label_font.pointSizeF() + 1.0);
    setFont(label_font);
    set_title_mode(false);
}

void QUI_Label::set_title_mode(bool enabled)
{
    if (enabled)
    {
        setStyleSheet("font-weight: 600; color: rgb(241, 241, 241);");
        return;
    }

    setStyleSheet("color: rgb(241, 241, 241);");
}

QUI_LineEdit::QUI_LineEdit(QWidget *parent)
    : QLineEdit(parent)
{
    initialize();
}

QUI_LineEdit::QUI_LineEdit(const QString &text, QWidget *parent)
    : QUI_LineEdit(parent)
{
    sync_text(text);
}

void QUI_LineEdit::sync_text(const QString &text)
{
    if (this->text() == text)
    {
        return;
    }

    setText(text);
    m_last_valid_text = text;
}

void QUI_LineEdit::set_value_mode(Value_Mode mode)
{
    m_value_mode = mode;
}

void QUI_LineEdit::set_integer_mode()
{
    set_value_mode(Value_Mode::Integer);
}

void QUI_LineEdit::set_double_mode()
{
    set_value_mode(Value_Mode::Double);
}

void QUI_LineEdit::set_string_mode()
{
    set_value_mode(Value_Mode::String);
}

QUI_LineEdit::Value_Mode QUI_LineEdit::value_mode() const
{
    return m_value_mode;
}

bool QUI_LineEdit::set_numeric_range(double minimum, double maximum)
{
    if (!std::isfinite(minimum) || !std::isfinite(maximum) || minimum > maximum)
    {
        return false;
    }

    m_numeric_range = qMakePair(minimum, maximum);
    return true;
}

void QUI_LineEdit::clear_numeric_range()
{
    m_numeric_range.reset();
}

bool QUI_LineEdit::set_unit_conversion(const QString &display_unit, const QString &storage_unit)
{
    if (!UnitSystem::are_compatible(display_unit, storage_unit))
    {
        m_display_unit.clear();
        m_storage_unit.clear();
        return false;
    }

    m_display_unit = display_unit.trimmed();
    m_storage_unit = storage_unit.trimmed();
    sync_from_binding();
    return true;
}

QString QUI_LineEdit::display_unit() const
{
    return m_display_unit;
}

QString QUI_LineEdit::storage_unit() const
{
    return m_storage_unit;
}

void QUI_LineEdit::bind_value(int *value)
{
    m_bound_value = value;
    m_value_mode = Value_Mode::Integer;
    sync_from_binding();
}

void QUI_LineEdit::bind_value(float *value)
{
    m_bound_value = value;
    m_value_mode = Value_Mode::Double;
    sync_from_binding();
}

void QUI_LineEdit::bind_value(double *value)
{
    m_bound_value = value;
    m_value_mode = Value_Mode::Double;
    sync_from_binding();
}

void QUI_LineEdit::bind_value(QString *value)
{
    m_bound_value = value;
    m_value_mode = Value_Mode::String;
    sync_from_binding();
}

void QUI_LineEdit::set_allow_empty_string(bool allow)
{
    m_allow_empty_string = allow;
}

bool QUI_LineEdit::commit()
{
    QString normalized;
    QString error_message;

    switch (m_value_mode)
    {
    case Value_Mode::Integer:
    {
        double numeric_value = 0.0;
        if (!commit_numeric_value(numeric_value, normalized, error_message))
        {
            mark_invalid(error_message);
            return false;
        }

        const double rounded_value = std::round(numeric_value);
        if (std::abs(numeric_value - rounded_value) > kIntegerTolerance)
        {
            mark_invalid("Integer input required.");
            return false;
        }

        bool conversion_ok = true;
        const double storage_value = m_display_unit.isEmpty()
            ? rounded_value
            : UnitSystem::convert(rounded_value, m_display_unit, m_storage_unit, &conversion_ok);
        if (!conversion_ok || !std::isfinite(storage_value))
        {
            mark_invalid("Unit conversion failed.");
            return false;
        }

        const int final_value = static_cast<int>(std::round(storage_value));
        if (auto *bound_value = std::get_if<int *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
        {
            **bound_value = final_value;
        }

        normalized = QString::number(final_value);
        break;
    }
    case Value_Mode::Double:
    {
        double numeric_value = 0.0;
        if (!commit_numeric_value(numeric_value, normalized, error_message))
        {
            mark_invalid(error_message);
            return false;
        }

        bool conversion_ok = true;
        const double storage_value = m_display_unit.isEmpty()
            ? numeric_value
            : UnitSystem::convert(numeric_value, m_display_unit, m_storage_unit, &conversion_ok);
        if (!conversion_ok || !std::isfinite(storage_value))
        {
            mark_invalid("Unit conversion failed.");
            return false;
        }

        if (auto *bound_value = std::get_if<double *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
        {
            **bound_value = storage_value;
        }
        else if (auto *bound_value = std::get_if<float *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
        {
            **bound_value = static_cast<float>(storage_value);
        }

        normalized = format_double_value(numeric_value);
        break;
    }
    case Value_Mode::String:
    {
        if (!commit_string_value(normalized, error_message))
        {
            mark_invalid(error_message);
            return false;
        }

        if (auto *bound_value = std::get_if<QString *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
        {
            **bound_value = normalized;
        }
        break;
    }
    }

    clear_invalid_state();
    setText(normalized);
    m_last_valid_text = normalized;
    emit value_committed();
    return true;
}

void QUI_LineEdit::initialize()
{
    setClearButtonEnabled(true);
    setMinimumHeight(30);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setStyleSheet(line_edit_style_sheet(false));
    connect(this, &QLineEdit::editingFinished, this, [this]() {
        commit();
    });
    m_last_valid_text = text();
}

void QUI_LineEdit::sync_from_binding()
{
    if (auto *bound_value = std::get_if<int *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
    {
        sync_numeric_value(static_cast<double>(**bound_value));
        return;
    }

    if (auto *bound_value = std::get_if<float *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
    {
        sync_numeric_value(static_cast<double>(**bound_value));
        return;
    }

    if (auto *bound_value = std::get_if<double *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
    {
        sync_numeric_value(**bound_value);
        return;
    }

    if (auto *bound_value = std::get_if<QString *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
    {
        sync_text(**bound_value);
    }
}

void QUI_LineEdit::restore_last_valid_text()
{
    setText(m_last_valid_text);
    selectAll();
}

void QUI_LineEdit::mark_invalid(const QString &reason)
{
    setToolTip(reason);
    setStyleSheet(line_edit_style_sheet(true));
    restore_last_valid_text();
    emit value_rejected(reason);
}

void QUI_LineEdit::clear_invalid_state()
{
    setToolTip(QString());
    setStyleSheet(line_edit_style_sheet(false));
}

bool QUI_LineEdit::commit_string_value(QString &normalized, QString &error_message)
{
    normalized = text().trimmed();
    if (!m_allow_empty_string && normalized.isEmpty())
    {
        error_message = "String input cannot be empty.";
        return false;
    }

    if (m_allow_empty_string && text().isEmpty())
    {
        normalized.clear();
        return true;
    }

    return true;
}

bool QUI_LineEdit::commit_numeric_value(double &numeric_value,
                                        QString &normalized,
                                        QString &error_message) const
{
    if (!evaluate_expression(text(), numeric_value, error_message))
    {
        return false;
    }

    if (m_numeric_range.has_value() &&
        (numeric_value < m_numeric_range->first || numeric_value > m_numeric_range->second))
    {
        error_message = QString("Value must be between %1 and %2.")
                            .arg(format_double_value(m_numeric_range->first),
                                 format_double_value(m_numeric_range->second));
        return false;
    }

    normalized = format_double_value(numeric_value);
    return true;
}

QUI_FieldRow::QUI_FieldRow(const QString &label_text, QWidget *parent)
    : QWidget(parent)
{
    initialize();
    set_label_text(label_text);
}

QUI_FieldRow::QUI_FieldRow(const QString &label_text, const QString &unit_text, QWidget *parent)
    : QUI_FieldRow(label_text, parent)
{
    set_unit_text(unit_text);
}

void QUI_FieldRow::set_compact(bool compact)
{
    if (compact)
    {
        m_layout->setContentsMargins(4, 2, 4, 2);
        m_layout->setSpacing(6);
        return;
    }

    m_layout->setContentsMargins(8, 4, 8, 4);
    m_layout->setSpacing(10);
}

void QUI_FieldRow::set_label_text(const QString &text)
{
    m_label_text = text.trimmed();
    update_label_display();
}

QString QUI_FieldRow::label_text() const
{
    return m_label_text;
}

void QUI_FieldRow::set_unit_text(const QString &text)
{
    const QString semantic_unit = text.trimmed();
    m_unit_text = UnitSystem::preferred_display_unit(semantic_unit);

    const Unit_Definition semantic_definition = UnitSystem::definition(semantic_unit);
    m_storage_unit = semantic_unit;
    if (semantic_definition.dimension == Unit_Dimension::Length)
    {
        m_storage_unit = UnitSystem::base_unit(Unit_Dimension::Length);
    }
    else if (semantic_definition.dimension == Unit_Dimension::Temperature)
    {
        m_storage_unit = UnitSystem::base_unit(Unit_Dimension::Temperature);
    }
    else if (semantic_definition.dimension != Unit_Dimension::Angle &&
             !semantic_definition.symbol.isEmpty())
    {
        m_storage_unit = UnitSystem::base_unit(semantic_definition.dimension);
    }

    if (UnitSystem::is_supported(m_unit_text) && UnitSystem::is_supported(m_storage_unit) &&
        UnitSystem::are_compatible(m_unit_text, m_storage_unit))
    {
        m_primary_editor->set_unit_conversion(m_unit_text, m_storage_unit);
        m_secondary_editor->set_unit_conversion(m_unit_text, m_storage_unit);
    }
    update_label_display();
}

QString QUI_FieldRow::unit_text() const
{
    return m_unit_text;
}

void QUI_FieldRow::set_layout_mode(Layout_Mode mode)
{
    m_layout_mode = mode;
    update_layout_mode();
}

QUI_FieldRow::Layout_Mode QUI_FieldRow::layout_mode() const
{
    return m_layout_mode;
}

void QUI_FieldRow::set_label_width(int width)
{
    m_label->setMinimumWidth(width);
    m_label->setMaximumWidth(width);
}

void QUI_FieldRow::set_secondary_editor_visible(bool visible)
{
    m_show_secondary_editor = visible;
    update_layout_mode();
}

QUI_Label *QUI_FieldRow::label_widget() const
{
    return m_label;
}

QUI_LineEdit *QUI_FieldRow::primary_editor() const
{
    return m_primary_editor;
}

QUI_LineEdit *QUI_FieldRow::secondary_editor() const
{
    return m_secondary_editor;
}

void QUI_FieldRow::initialize()
{
    m_layout = new QHBoxLayout(this);
    m_label = new QUI_Label(QString(), this);
    m_primary_editor = new QUI_LineEdit(this);
    m_secondary_editor = new QUI_LineEdit(this);

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_layout->addWidget(m_label, 0);
    m_layout->addWidget(m_primary_editor, 1);
    m_layout->addWidget(m_secondary_editor, 1);

    set_label_width(180);
    set_compact(true);
    set_layout_mode(Layout_Mode::SingleValue);
}

void QUI_FieldRow::update_label_display()
{
    if (m_unit_text.isEmpty())
    {
        m_label->setText(m_label_text);
        return;
    }

    m_label->setText(QString("%1 [%2]").arg(m_label_text, m_unit_text));
}

void QUI_FieldRow::update_layout_mode()
{
    const bool is_range_mode = (m_layout_mode == Layout_Mode::RangeValue);
    m_secondary_editor->setVisible(is_range_mode && m_show_secondary_editor);
}

QUI_ComboBox::QUI_ComboBox(QWidget *parent)
    : QComboBox(parent)
{
    initialize();
}

void QUI_ComboBox::set_options(const QStringList &options)
{
    bool options_changed = count() != options.size();
    if (!options_changed)
    {
        for (int index = 0; index < options.size(); ++index)
        {
            if (itemText(index) != options.at(index))
            {
                options_changed = true;
                break;
            }
        }
    }

    const QSignalBlocker blocker(this);
    if (options_changed)
    {
        clear();
        addItems(options);
    }
    sync_from_binding();
}

void QUI_ComboBox::add_option(const QString &text, const QVariant &data)
{
    const QSignalBlocker blocker(this);
    addItem(text, data);
    sync_from_binding();
}

void QUI_ComboBox::bind_current_index(int *value)
{
    m_bound_value = value;
    sync_from_binding();
}

void QUI_ComboBox::bind_current_text(QString *value)
{
    m_bound_value = value;
    sync_from_binding();
}

void QUI_ComboBox::sync_from_binding()
{
    const QSignalBlocker blocker(this);
    if (auto *bound_value = std::get_if<int *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
    {
        if (**bound_value >= 0 && **bound_value < count())
        {
            setCurrentIndex(**bound_value);
        }
        return;
    }

    if (auto *bound_value = std::get_if<QString *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
    {
        const int target_index = findText(**bound_value);
        if (target_index >= 0)
        {
            setCurrentIndex(target_index);
        }
    }
}

void QUI_ComboBox::initialize()
{
    setMinimumHeight(30);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setStyleSheet(combo_box_style_sheet());

    connect(this,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int)
            {
                commit_binding();
            });
}

void QUI_ComboBox::commit_binding()
{
    if (auto *bound_value = std::get_if<int *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
    {
        **bound_value = currentIndex();
    }
    else if (auto *bound_value = std::get_if<QString *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
    {
        **bound_value = currentText();
    }

    emit selection_committed();
}

QUI_SpinBox::QUI_SpinBox(QWidget *parent)
    : QSpinBox(parent)
{
    initialize();
}

void QUI_SpinBox::bind_value(int *value)
{
    m_bound_value = value;
    sync_from_binding();
}

void QUI_SpinBox::sync_from_binding()
{
    if (m_bound_value != nullptr)
    {
        setValue(*m_bound_value);
    }
}

void QUI_LineEdit::sync_bound_value()
{
    sync_from_binding();
}

void QUI_LineEdit::sync_numeric_value(double storage_value)
{
    if (m_display_unit.isEmpty())
    {
        sync_text(format_double_value(storage_value));
        return;
    }

    bool conversion_ok = false;
    const double display_value = UnitSystem::convert(
        storage_value, m_storage_unit, m_display_unit, &conversion_ok);
    sync_text(conversion_ok ? format_double_value(display_value) : QString());
}

bool QUI_LineEdit::numeric_value_in_storage(double &storage_value, QString *error_message) const
{
    QString local_error;
    double display_value = 0.0;
    QString normalized;
    if (!commit_numeric_value(display_value, normalized, local_error))
    {
        if (error_message != nullptr)
        {
            *error_message = local_error;
        }
        return false;
    }

    bool conversion_ok = true;
    storage_value = m_display_unit.isEmpty()
        ? display_value
        : UnitSystem::convert(display_value, m_display_unit, m_storage_unit, &conversion_ok);
    if (!conversion_ok || !std::isfinite(storage_value))
    {
        if (error_message != nullptr)
        {
            *error_message = "Unit conversion failed.";
        }
        return false;
    }

    if (error_message != nullptr)
    {
        error_message->clear();
    }
    return true;
}

void QUI_SpinBox::initialize()
{
    setMinimumHeight(30);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setStyleSheet(spin_box_style_sheet());

    connect(this,
            QOverload<int>::of(&QSpinBox::valueChanged),
            this,
            [this](int value)
            {
                commit_binding(value);
            });
}

void QUI_SpinBox::commit_binding(int value)
{
    if (m_bound_value != nullptr)
    {
        *m_bound_value = value;
    }

    emit value_committed(value);
}

QUI_CheckBox::QUI_CheckBox(const QString &text, QWidget *parent)
    : QCheckBox(text, parent)
{
    initialize();
}

void QUI_CheckBox::bind_value(bool *value)
{
    m_bound_value = value;
    sync_from_binding();
}

void QUI_CheckBox::sync_from_binding()
{
    const QSignalBlocker blocker(this);
    if (m_bound_value != nullptr)
    {
        setChecked(*m_bound_value);
    }
}

void QUI_CheckBox::initialize()
{
    setStyleSheet(check_box_style_sheet());
    connect(this, &QCheckBox::toggled, this, [this](bool checked)
    {
        commit_binding(checked);
    });
}

void QUI_CheckBox::commit_binding(bool checked)
{
    if (m_bound_value != nullptr)
    {
        *m_bound_value = checked;
    }

    emit value_committed(checked);
}

QUI_PushButton::QUI_PushButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent)
{
    initialize();
}

void QUI_PushButton::set_accent_mode(bool enabled)
{
    m_accent_mode = enabled;
    update_style();
}

void QUI_PushButton::initialize()
{
    setMinimumHeight(32);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    update_style();
}

void QUI_PushButton::update_style()
{
    setStyleSheet(push_button_style_sheet(m_accent_mode));
}

QUI_RadioGroup::QUI_RadioGroup(const QString &title, QWidget *parent)
    : QWidget(parent)
{
    Q_UNUSED(title);
    initialize();
}

void QUI_RadioGroup::clear_options()
{
    m_next_option_id = 0;

    const QList<QAbstractButton *> buttons = m_button_group->buttons();
    for (QAbstractButton *button : buttons)
    {
        m_button_group->removeButton(button);
        m_layout->removeWidget(button);
        delete button;
    }
}

void QUI_RadioGroup::set_options(const QStringList &options)
{
    clear_options();
    for (const QString &option : options)
    {
        add_option(option);
    }
    sync_from_binding();
}

void QUI_RadioGroup::add_option(const QString &text, int id)
{
    const int final_id = (id >= 0) ? id : m_next_option_id;
    auto *button = new QRadioButton(text, this);
    m_button_group->addButton(button, final_id);
    m_layout->addWidget(button);
    ++m_next_option_id;
}

void QUI_RadioGroup::bind_checked_id(int *value)
{
    m_bound_checked_id = value;
    sync_from_binding();
}

void QUI_RadioGroup::sync_from_binding()
{
    const QSignalBlocker blocker(m_button_group);
    if (m_bound_checked_id == nullptr)
    {
        return;
    }

    if (QAbstractButton *button = m_button_group->button(*m_bound_checked_id))
    {
        button->setChecked(true);
    }
}

int QUI_RadioGroup::checked_id() const
{
    return m_button_group->checkedId();
}

void QUI_RadioGroup::set_checked_id(int checked_id)
{
    if (QAbstractButton *button = m_button_group->button(checked_id))
    {
        button->setChecked(true);
    }
}

void QUI_RadioGroup::initialize()
{
    m_layout = new QHBoxLayout(this);
    m_button_group = new QButtonGroup(this);
    m_button_group->setExclusive(true);

    m_layout->setContentsMargins(12, 6, 12, 6);
    m_layout->setSpacing(12);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(radio_group_panel_style_sheet());
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    connect(m_button_group,
            QOverload<int>::of(&QButtonGroup::idClicked),
            this,
            [this](int checked_id)
            {
                commit_binding(checked_id);
            });
}

void QUI_RadioGroup::commit_binding(int checked_id)
{
    if (m_bound_checked_id != nullptr)
    {
        *m_bound_checked_id = checked_id;
    }

    emit value_committed(checked_id);
}
