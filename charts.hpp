#ifndef CHARTS_H
#define CHARTS_H

// Charts
#include <qtconfigmacros.h>
#include <QApplication>
#include <QColor>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QRandomGenerator>
#include <QtCharts>
#include <utility>
#include <vector>

// Utility function to generate a random color
inline QColor generateRandomColor() {
    return {QRandomGenerator::global()->bounded(256),
            QRandomGenerator::global()->bounded(256),
            QRandomGenerator::global()->bounded(256)};
}

// Abstract Chart Class - Base for all Chart Types
class AbstractChart {
public:
    [[nodiscard]] virtual QWidget* widget() const = 0;
    virtual ~AbstractChart()                      = default;
};

// Bar Chart Class
class BarChart : public AbstractChart {
public:
    /**
     * @brief Creates a bar chart with the specified set name, title, and x-axis labels.
     * @param setName Name of the bar set
     * @param chartTitle Title of the chart
     * @param xLabels Labels for the x-axis categories
     */
    BarChart(QString chartTitle, const QStringList& xLabels)
        : chart(new QChart()),
          axisX(new QBarCategoryAxis(chart)),
          axisY(new QValueAxis(chart)),
          chartView(new QChartView(chart)),
          title(std::move(chartTitle)),
          xLabels(xLabels) {

        chart->setTitle(title);

        axisX->append(xLabels);
        chart->addAxis(axisX, Qt::AlignBottom);

        chart->addAxis(axisY, Qt::AlignLeft);

        chartView->setRenderHint(QPainter::Antialiasing);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);
    }

    void addSeries(const QString& setName, const std::vector<qreal>& data, const QColor& color = QColor()) {
        auto* set = new QBarSet(setName);
        for (auto value : data) {
            *set << value;
        }
        series << new QBarSeries();
        series.last()->append(set);
        chart->addSeries(series.last());
        series.last()->attachAxis(axisX);
        series.last()->attachAxis(axisY);
        if (color.isValid()) {
            set->setColor(color);
        }
    }

    // Set the range for the y-axis
    void setYRange(qreal min, qreal max) {
        axisY->setRange(min, max);
    }

    // Returns the QChartView widget.
    [[nodiscard]] QWidget* widget() const override {
        return chartView;
    }

    void setTheme(QChart::ChartTheme theme) {
        chart->setTheme(theme);
    }

    void setChartTitle(const QString& newTitle) {
        title = newTitle;
        chart->setTitle(title);
    }

    void setXLabels(const QStringList& newXLabels) {
        xLabels = newXLabels;
        axisX->clear();
        axisX->append(xLabels);
    }

    void setAnimationOptions(QChart::AnimationOptions options) {
        chart->setAnimationOptions(options);
    }

    void setLegendVisible(bool visible) {
        chart->legend()->setVisible(visible);
    }

    void setLegendAlignment(Qt::Alignment alignment) {
        chart->legend()->setAlignment(alignment);
    }

    ~BarChart() override {
        delete chart;
    }

private:
    QList<QBarSeries*> series;
    QChart* chart           = nullptr;
    QBarCategoryAxis* axisX = nullptr;
    QValueAxis* axisY       = nullptr;
    QChartView* chartView   = nullptr;
    QString title;
    QStringList xLabels;
};

// Line Chart Class
class LineChart : public AbstractChart {
public:
    LineChart(QString chartTitle)
        : chart(new QChart()),
          axisX(new QValueAxis(chart)),
          axisY(new QValueAxis(chart)),
          chartView(new QChartView(chart)),
          title(std::move(chartTitle)) {
        // Set the chart title
        chart->setTitle(title);

        // Configure and attach the x-axis (bottom-aligned)
        chart->addAxis(axisX, Qt::AlignBottom);

        // Configure and attach the y-axis (left-aligned)
        chart->addAxis(axisY, Qt::AlignLeft);

        // Enable antialiasing for smoother rendering
        chartView->setRenderHint(QPainter::Antialiasing);
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);
    }

    // Plot data as a vector of points with x and y coordinates
    void addSeries(const QString& seriesName, const std::vector<QPointF>& data, const QColor& color = QColor()) {
        auto* series = new QLineSeries();
        series->setName(seriesName);
        for (const auto& point : data) {
            *series << point;  // Add new points
        }
        chart->addSeries(series);
        series->attachAxis(axisX);
        series->attachAxis(axisY);

        if (color.isValid()) {
            QPen pen = series->pen();
            pen.setColor(color);
            series->setPen(pen);
        }
    }

    // Set the range for the y-axis
    void setYRange(qreal min, qreal max) {
        axisY->setRange(min, max);
    }

    void setXRange(qreal min, qreal max) {
        axisX->setRange(min, max);
    }

    // Return the chart widget for display
    [[nodiscard]] QWidget* widget() const override {
        return chartView;
    }

    void setTheme(QChart::ChartTheme theme) {
        chart->setTheme(theme);
    }

    void setChartTitle(const QString& newTitle) {
        title = newTitle;
        chart->setTitle(title);
    }

    void setAnimationOptions(QChart::AnimationOptions options) {
        chart->setAnimationOptions(options);
    }

    void setLegendVisible(bool visible) {
        chart->legend()->setVisible(visible);
    }

    void setLegendAlignment(Qt::Alignment alignment) {
        chart->legend()->setAlignment(alignment);
    }

    ~LineChart() override {
        delete chart;
    }

private:
    QChart* chart         = nullptr;  // Chart object to manage the series and axes
    QValueAxis* axisX     = nullptr;  // Numerical x-axis
    QValueAxis* axisY     = nullptr;  // Numerical y-axis
    QChartView* chartView = nullptr;  // Widget to display the chart
    QString title;
};

// Spline Chart Class
class SplineChart : public AbstractChart {
public:
    SplineChart(QString chartTitle)
        : chart(new QChart()),
          axisX(new QValueAxis(chart)),
          axisY(new QValueAxis(chart)),
          chartView(new QChartView(chart)),
          title(std::move(chartTitle)) {
        // Set the chart title
        chart->setTitle(title);

        // Configure and attach the x-axis (bottom-aligned)
        chart->addAxis(axisX, Qt::AlignBottom);

        // Configure and attach the y-axis (left-aligned)
        chart->addAxis(axisY, Qt::AlignLeft);

        // Enable antialiasing for smoother rendering
        chartView->setRenderHint(QPainter::Antialiasing);
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);
    }

    void addSeries(const QString& seriesName, const std::vector<QPointF>& data, const QColor& color = QColor()) {
        auto* series = new QSplineSeries();
        series->setName(seriesName);
        for (const auto& point : data) {
            *series << point;  // Add new points
        }
        chart->addSeries(series);
        series->attachAxis(axisX);
        series->attachAxis(axisY);

        if (color.isValid()) {
            QPen pen = series->pen();
            pen.setColor(color);
            series->setPen(pen);
        }
    }

    // Set the range for the y-axis
    void setYRange(qreal min, qreal max) {
        axisY->setRange(min, max);
    }

    void setXRange(qreal min, qreal max) {
        axisX->setRange(min, max);
    }

    // Return the chart widget for display
    [[nodiscard]] QWidget* widget() const override {
        return chartView;
    }

    void setTheme(QChart::ChartTheme theme) {
        chart->setTheme(theme);
    }

    void setChartTitle(const QString& newTitle) {
        title = newTitle;
        chart->setTitle(title);
    }

    void setAnimationOptions(QChart::AnimationOptions options) {
        chart->setAnimationOptions(options);
    }

    void setLegendVisible(bool visible) {
        chart->legend()->setVisible(visible);
    }

    void setLegendAlignment(Qt::Alignment alignment) {
        chart->legend()->setAlignment(alignment);
    }

    ~SplineChart() override {
        delete chart;
    }

private:
    QChart* chart         = nullptr;  // Chart object to manage the series and axes
    QValueAxis* axisX     = nullptr;  // Numerical x-axis
    QValueAxis* axisY     = nullptr;  // Numerical y-axis
    QChartView* chartView = nullptr;  // Widget to display the chart
    QString title;
};

// Pie Chart Class
class PieChart : public AbstractChart {
public:
    PieChart(QString chartTitle)
        : chart(new QChart()),
          series(new QPieSeries(chart)),
          chartView(new QChartView(chart)),
          title(std::move(chartTitle)) {

        // Add the series to the chart
        chart->addSeries(series);

        // Set the chart title
        chart->setTitle(title);

        series->setHoleSize(0.25);

        // Enable antialiasing for smoother rendering
        chartView->setRenderHint(QPainter::Antialiasing);
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignBottom);
    }

    // Plot data as a vector of label-value pairs
    void plot(const std::vector<std::pair<QString, qreal>>& data) {
        series->clear();  // Remove existing slices
        for (const auto& pair : data) {
            // Create and append a new slice with label and value
            auto* slice = new QPieSlice(pair.first, pair.second);
            series->append(slice);
            slice->setBrush(generateRandomColor());
            slice->setLabelVisible(true);
        }
    }

    // Return the chart Widget for display
    [[nodiscard]] QWidget* widget() const override {
        return chartView;
    }

    void setChartTitle(const QString& newTitle) {
        title = newTitle;
        chart->setTitle(title);
    }

    void setTheme(QChart::ChartTheme theme) {
        chart->setTheme(theme);
    }

    void setAnimationOptions(QChart::AnimationOptions options) {
        chart->setAnimationOptions(options);
    }

    void setLegendVisible(bool visible) {
        chart->legend()->setVisible(visible);
    }

    void setLegendAlignment(Qt::Alignment alignment) {
        chart->legend()->setAlignment(alignment);
    }

    ~PieChart() override {
        delete chart;
    }

private:
    QChart* chart         = nullptr;  // Chart object to manage the series
    QPieSeries* series    = nullptr;  // Series to hold pie slices
    QChartView* chartView = nullptr;  // Widget to display the chart
    QString title;
};

#endif /* CHARTS_H */
