#include "qUI_components.h"

#include <QFont>
#include <QSizePolicy>
#include <QLocale>

#include <tinyexpr.h>

#include <array>
#include <cmath>
#include <memory>

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
            "  border: 1px solid rgb(200, 70, 70);"
            "  border-radius: 6px;"
            "  background: rgb(255, 248, 248);"
            "  padding: 4px 8px;"
            "}");
    }

    return QString(
        "QLineEdit {"
        "  border: 1px solid rgb(205, 211, 220);"
        "  border-radius: 6px;"
        "  background: white;"
        "  padding: 4px 8px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid rgb(120, 156, 214);"
        "}");
}
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
        setStyleSheet("font-weight: 600;");
        return;
    }

    setStyleSheet(QString());
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

void QUI_LineEdit::bind_value(int *value)
{
    m_bound_value = value;
    m_value_mode = Value_Mode::Integer;
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

        const int final_value = static_cast<int>(rounded_value);
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

        if (auto *bound_value = std::get_if<double *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
        {
            **bound_value = numeric_value;
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
        sync_text(QString::number(**bound_value));
        return;
    }

    if (auto *bound_value = std::get_if<double *>(&m_bound_value); bound_value != nullptr && *bound_value != nullptr)
    {
        sync_text(format_double_value(**bound_value));
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
    m_unit_text = text.trimmed();
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
