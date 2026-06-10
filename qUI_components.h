#ifndef QUI_COMPONENTS_H
#define QUI_COMPONENTS_H

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QWidget>
#include <variant>

class QUI_Label : public QLabel
{
public:
    explicit QUI_Label(const QString &text = QString(), QWidget *parent = nullptr);

    void set_title_mode(bool enabled = true);
};

class QUI_LineEdit : public QLineEdit
{
    Q_OBJECT

public:
    enum class Value_Mode
    {
        Integer,
        Double,
        String
    };

    explicit QUI_LineEdit(QWidget *parent = nullptr);
    explicit QUI_LineEdit(const QString &text, QWidget *parent = nullptr);

    void sync_text(const QString &text);
    void set_value_mode(Value_Mode mode);
    void set_integer_mode();
    void set_double_mode();
    void set_string_mode();
    Value_Mode value_mode() const;

    void bind_value(int *value);
    void bind_value(double *value);
    void bind_value(QString *value);

    void set_allow_empty_string(bool allow);
    bool commit();

signals:
    void value_committed();
    void value_rejected(const QString &reason);

private:
    using Bound_Value = std::variant<std::monostate, int *, double *, QString *>;

    void initialize();
    void sync_from_binding();
    void restore_last_valid_text();
    void mark_invalid(const QString &reason);
    void clear_invalid_state();

    bool commit_string_value(QString &normalized, QString &error_message);
    bool commit_numeric_value(double &numeric_value, QString &normalized, QString &error_message) const;

    Bound_Value m_bound_value;
    Value_Mode m_value_mode = Value_Mode::String;
    QString m_last_valid_text;
    bool m_allow_empty_string = true;
};

class QUI_FieldRow : public QWidget
{
public:
    enum class Layout_Mode
    {
        SingleValue,
        RangeValue
    };

    explicit QUI_FieldRow(const QString &label_text = QString(), QWidget *parent = nullptr);
    QUI_FieldRow(const QString &label_text, const QString &unit_text, QWidget *parent = nullptr);

    void set_compact(bool compact = true);
    void set_label_text(const QString &text);
    QString label_text() const;
    void set_unit_text(const QString &text);
    QString unit_text() const;

    void set_layout_mode(Layout_Mode mode);
    Layout_Mode layout_mode() const;

    void set_label_width(int width);
    void set_secondary_editor_visible(bool visible);

    QUI_Label *label_widget() const;
    QUI_LineEdit *primary_editor() const;
    QUI_LineEdit *secondary_editor() const;

private:
    void initialize();
    void update_label_display();
    void update_layout_mode();

    QHBoxLayout *m_layout = nullptr;
    QUI_Label *m_label = nullptr;
    QUI_LineEdit *m_primary_editor = nullptr;
    QUI_LineEdit *m_secondary_editor = nullptr;
    QString m_label_text;
    QString m_unit_text;
    Layout_Mode m_layout_mode = Layout_Mode::SingleValue;
    bool m_show_secondary_editor = true;
};

#endif // QUI_COMPONENTS_H
