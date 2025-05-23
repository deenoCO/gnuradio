/* -*- c++ -*- */
/*
 * Copyright 2008-2011,2014 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#ifndef FREQUENCY_DISPLAY_PLOT_C
#define FREQUENCY_DISPLAY_PLOT_C

#include <gnuradio/qtgui/FrequencyDisplayPlot.h>

#include <gnuradio/qtgui/qtgui_types.h>
#include <qwt_scale_draw.h>
#include <QColor>
#include <cmath>

/***********************************************************************
 * Widget to provide mouse pointer coordinate text
 **********************************************************************/
class FreqDisplayZoomer : public QwtPlotZoomer, public FreqOffsetAndPrecisionClass
{
public:
    FreqDisplayZoomer(QWidget* canvas, const unsigned int freqPrecision)
        : QwtPlotZoomer(canvas), FreqOffsetAndPrecisionClass(freqPrecision)
    {
        setTrackerMode(QwtPicker::AlwaysOn);
    }

    virtual void updateTrackerText() { updateDisplay(); }

    void setUnitType(const std::string& type) { d_unitType = type; }

    void setYUnit(const std::string& unit) { d_y_unit = unit; }

protected:
    using QwtPlotZoomer::trackerText;
    QwtText trackerText(QPoint const& p) const override
    {
        QPointF dp = QwtPlotZoomer::invTransform(p);
        QwtText t(QString("%1 %2, %3 %4")
                      .arg(dp.x(), 0, 'f', getFrequencyPrecision())
                      .arg(d_unitType.c_str())
                      .arg(dp.y(), 0, 'f', 2)
                      .arg(d_y_unit.c_str()));
        return t;
    }

private:
    std::string d_unitType;
    std::string d_y_unit = "dB";
};


/***********************************************************************
 * Main frequency display plotter widget
 **********************************************************************/
FrequencyDisplayPlot::FrequencyDisplayPlot(int nplots, QWidget* parent)
    : DisplayPlot(nplots, parent)
{
    d_numPoints = 0;

    setAxisTitle(QwtPlot::xBottom, "Frequency (Hz)");
    setAxisScaleDraw(QwtPlot::xBottom, new FreqDisplayScaleDraw(0));

    setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
    setAxisScale(QwtPlot::yLeft, d_ymin, d_ymax);
    setAxisTitle(QwtPlot::yLeft, "Relative Gain (dB)");

    QList<QColor> default_colors;
    default_colors << QColor(Qt::blue) << QColor(Qt::red) << QColor(Qt::green)
                   << QColor(Qt::black) << QColor(Qt::cyan) << QColor(Qt::magenta)
                   << QColor(Qt::yellow) << QColor(Qt::gray) << QColor(Qt::darkRed)
                   << QColor(Qt::darkGreen) << QColor(Qt::darkBlue)
                   << QColor(Qt::darkGray);

    // Create a curve for each input
    // Automatically deleted when parent is deleted
    for (unsigned int i = 0; i < d_nplots; ++i) {
        d_ydata.emplace_back(d_numPoints);

        d_plot_curve.push_back(new QwtPlotCurve(QString("Data %1").arg(i)));
        d_plot_curve[i]->attach(this);

        QwtSymbol* symbol = new QwtSymbol(QwtSymbol::NoSymbol,
                                          QBrush(default_colors[i]),
                                          QPen(default_colors[i]),
                                          QSize(7, 7));

        d_plot_curve[i]->setRawSamples(d_xdata.data(), d_ydata[i].data(), d_numPoints);
        d_plot_curve[i]->setSymbol(symbol);
        setLineColor(i, default_colors[i]);
    }

    // Create min/max plotter curves
    d_min_fft_plot_curve = new QwtPlotCurve("Min Hold");
    d_min_fft_plot_curve->attach(this);
    const QColor default_min_fft_color = Qt::magenta;
    setMinFFTColor(default_min_fft_color);
    d_min_fft_plot_curve->setRawSamples(
        d_xdata.data(), d_min_fft_data.data(), d_numPoints);
    d_min_fft_plot_curve->setVisible(false);
    d_min_fft_plot_curve->setZ(0);

    d_max_fft_plot_curve = new QwtPlotCurve("Max Hold");
    d_max_fft_plot_curve->attach(this);
    QColor default_max_fft_color = Qt::darkYellow;
    setMaxFFTColor(default_max_fft_color);
    d_max_fft_plot_curve->setRawSamples(
        d_xdata.data(), d_max_fft_data.data(), d_numPoints);
    d_max_fft_plot_curve->setVisible(false);
    d_max_fft_plot_curve->setZ(0);

    d_lower_intensity_marker = new QwtPlotMarker();
    d_lower_intensity_marker->setLineStyle(QwtPlotMarker::HLine);
    QColor default_marker_lower_intensity_color = Qt::cyan;
    setMarkerLowerIntensityColor(default_marker_lower_intensity_color);
    d_lower_intensity_marker->attach(this);

    d_upper_intensity_marker = new QwtPlotMarker();
    d_upper_intensity_marker->setLineStyle(QwtPlotMarker::HLine);
    QColor default_marker_upper_intensity_color = Qt::green;
    setMarkerUpperIntensityColor(default_marker_upper_intensity_color);
    d_upper_intensity_marker->attach(this);

    std::fill(std::begin(d_xdata), std::end(d_xdata), 0);

    for (int64_t number = 0; number < d_numPoints; number++) {
        d_min_fft_data[number] = 200.0;
        d_max_fft_data[number] = -280.0;
    }

    d_marker_peak_amplitude = new QwtPlotMarker();
    QColor default_marker_peak_amplitude_color = Qt::yellow;
    setMarkerPeakAmplitudeColor(default_marker_peak_amplitude_color);
    /// THIS CAUSES A PROBLEM!
    // d_marker_peak_amplitude->attach(this);

    d_marker_noise_floor_amplitude = new QwtPlotMarker();
    d_marker_noise_floor_amplitude->setLineStyle(QwtPlotMarker::HLine);
    QColor default_marker_noise_floor_amplitude_color = Qt::darkRed;
    setMarkerNoiseFloorAmplitudeColor(default_marker_noise_floor_amplitude_color);
    d_marker_noise_floor_amplitude->attach(this);

    d_marker_cf = new QwtPlotMarker();
    d_marker_cf->setLineStyle(QwtPlotMarker::VLine);
    QColor default_marker_cf_color = Qt::lightGray;
    setMarkerCFColor(default_marker_cf_color);
    d_marker_cf->attach(this);
    d_marker_cf->hide();

    d_peak_frequency = 0;
    d_peak_amplitude = -HUGE_VAL;

    d_noise_floor_amplitude = -HUGE_VAL;

    d_zoomer = new FreqDisplayZoomer(canvas(), 0);

    d_zoomer->setMousePattern(
        QwtEventPattern::MouseSelect2, Qt::RightButton, Qt::ControlModifier);
    d_zoomer->setMousePattern(QwtEventPattern::MouseSelect3, Qt::RightButton);


    const QColor default_zoomer_color(Qt::darkRed);
    setZoomerColor(default_zoomer_color);

    // Do this after the zoomer has been built
    _resetXAxisPoints();

    // Turn off min/max hold plots in legend
    QWidget* w;
    w = ((QwtLegend*)legend())->legendWidget(itemToInfo(d_min_fft_plot_curve));
    ((QwtLegendLabel*)w)->setChecked(true);
    ((QwtLegendLabel*)w)->setVisible(false);

    w = ((QwtLegend*)legend())->legendWidget(itemToInfo(d_max_fft_plot_curve));
    ((QwtLegendLabel*)w)->setChecked(true);
    ((QwtLegendLabel*)w)->setVisible(false);

    d_trigger_line = new QwtPlotMarker();
    d_trigger_line->setLineStyle(QwtPlotMarker::HLine);
    d_trigger_line->setLinePen(QPen(Qt::red, 0.6, Qt::DashLine));
    d_trigger_line->setRenderHint(QwtPlotItem::RenderAntialiased);
    d_trigger_line->setXValue(0.0);
    d_trigger_line->setYValue(0.0);

    replot();
}

FrequencyDisplayPlot::~FrequencyDisplayPlot() {}

void FrequencyDisplayPlot::setYaxis(double min, double max)
{
    // Get the new max/min values for the plot
    d_ymin = min;
    d_ymax = max;

    // Set the axis max/min to the new values
    setAxisScale(QwtPlot::yLeft, d_ymin, d_ymax);

    // Reset the base zoom level to the new axis scale set here.
    // But don't do it if we set the axis due to auto scaling.
    if (!d_autoscale_state)
        d_zoomer->setZoomBase();
}

double FrequencyDisplayPlot::getYMin() const { return d_ymin; }

double FrequencyDisplayPlot::getYMax() const { return d_ymax; }

void FrequencyDisplayPlot::setFrequencyRange(const double centerfreq,
                                             const double bandwidth,
                                             const double units,
                                             const std::string& strunits)
{
    double startFreq;
    double stopFreq = (centerfreq + bandwidth / 2.0f) / units;
    if (d_half_freq)
        startFreq = centerfreq / units;
    else
        startFreq = (centerfreq - bandwidth / 2.0f) / units;

    d_xdata_multiplier = units;

    bool reset = false;
    if ((startFreq != d_start_frequency) || (stopFreq != d_stop_frequency))
        reset = true;

    if (stopFreq > startFreq) {
        d_start_frequency = startFreq;
        d_stop_frequency = stopFreq;
        d_center_frequency = centerfreq / units;

        if ((axisScaleDraw(QwtPlot::xBottom) != NULL) && (d_zoomer != NULL)) {
            double display_units = ceil(log10(units) / 2.0);
            setAxisScaleDraw(QwtPlot::xBottom, new FreqDisplayScaleDraw(display_units));
            setAxisTitle(QwtPlot::xBottom,
                         QString("Frequency (%1)").arg(strunits.c_str()));

            if (reset) {
                _resetXAxisPoints();
                clearMaxData();
                clearMinData();
            }

            ((FreqDisplayZoomer*)d_zoomer)->setFrequencyPrecision(display_units);
            ((FreqDisplayZoomer*)d_zoomer)->setUnitType(strunits);
        }
    }
}


double FrequencyDisplayPlot::getStartFrequency() const { return d_start_frequency; }

double FrequencyDisplayPlot::getStopFrequency() const { return d_stop_frequency; }

void FrequencyDisplayPlot::replot()
{
    d_marker_noise_floor_amplitude->setYValue(d_noise_floor_amplitude);
    d_marker_peak_amplitude->setXValue(d_peak_frequency + d_start_frequency);

    // Make sure to take into account the start frequency
    //  if(d_useCenterFrequencyFlag){
    //    d_marker_peak_amplitude->setXValue((d_peak_frequency/1000.0) +
    //    d_start_frequency);
    //  }
    //  else{
    //    _markerPeakAmplitude->setXValue(d_peak_frequency + d_start_frequency);
    //  }
    d_marker_peak_amplitude->setYValue(d_peak_amplitude);

    QwtPlot::replot();
}

void FrequencyDisplayPlot::plotNewData(const std::vector<const double*> dataPoints,
                                       const int64_t numDataPoints,
                                       const double noiseFloorAmplitude,
                                       const double peakFrequency,
                                       const double peakAmplitude,
                                       const double timeInterval)
{
    int64_t _npoints_in = d_half_freq ? numDataPoints / 2 : numDataPoints;
    int64_t _in_index = d_half_freq ? _npoints_in : 0;

    if (!d_stop) {
        if (numDataPoints > 0) {
            if (_npoints_in != d_numPoints) {
                d_numPoints = _npoints_in;

                d_xdata.resize(d_numPoints);
                d_min_fft_data.resize(d_numPoints);
                d_max_fft_data.resize(d_numPoints);

                for (unsigned int i = 0; i < d_nplots; ++i) {
                    d_ydata[i].resize(d_numPoints);

                    d_plot_curve[i]->setRawSamples(
                        d_xdata.data(), d_ydata[i].data(), d_numPoints);
                }
                d_min_fft_plot_curve->setRawSamples(
                    d_xdata.data(), d_min_fft_data.data(), d_numPoints);
                d_max_fft_plot_curve->setRawSamples(
                    d_xdata.data(), d_max_fft_data.data(), d_numPoints);
                _resetXAxisPoints();
                clearMaxData();
                clearMinData();
            }

            double bottom = 1e20, top = -1e20;
            for (unsigned int n = 0; n < d_nplots; ++n) {

                memcpy(d_ydata[n].data(),
                       &(dataPoints[n][_in_index]),
                       _npoints_in * sizeof(double));

                for (int64_t point = 0; point < _npoints_in; point++) {
                    if (dataPoints[n][point + _in_index] < d_min_fft_data[point]) {
                        d_min_fft_data[point] = dataPoints[n][point + _in_index];
                    }
                    if (dataPoints[n][point + _in_index] > d_max_fft_data[point]) {
                        d_max_fft_data[point] = dataPoints[n][point + _in_index];
                    }

                    // Find overall top and bottom values in plot.
                    // Used for autoscaling y-axis.
                    if (dataPoints[n][point] < bottom) {
                        bottom = dataPoints[n][point];
                    }
                    if (dataPoints[n][point] > top) {
                        top = dataPoints[n][point];
                    }
                }
            }

            if (d_autoscale_state) {
                _autoScale(bottom, top);
                if (d_autoscale_shot) {
                    d_autoscale_state = false;
                    d_autoscale_shot = false;
                }
            }

            d_noise_floor_amplitude = noiseFloorAmplitude;
            d_peak_frequency = peakFrequency;
            d_peak_amplitude = peakAmplitude;

            setUpperIntensityLevel(d_peak_amplitude);

            replot();
        }
    }
}

void FrequencyDisplayPlot::plotNewData(const double* dataPoints,
                                       const int64_t numDataPoints,
                                       const double noiseFloorAmplitude,
                                       const double peakFrequency,
                                       const double peakAmplitude,
                                       const double timeInterval)
{
    std::vector<const double*> vecDataPoints;
    vecDataPoints.push_back(dataPoints);
    plotNewData(vecDataPoints,
                numDataPoints,
                noiseFloorAmplitude,
                peakFrequency,
                peakAmplitude,
                timeInterval);
}

void FrequencyDisplayPlot::clearMaxData()
{
    std::fill(std::begin(d_max_fft_data), std::end(d_max_fft_data), d_ymin);
}

void FrequencyDisplayPlot::clearMinData()
{
    std::fill(std::begin(d_min_fft_data), std::end(d_min_fft_data), d_ymax);
}

void FrequencyDisplayPlot::_autoScale(double bottom, double top)
{
    // Auto scale the y-axis with a margin of 10 dB on either side.
    d_ymin = bottom - 10;
    d_ymax = top + 10;
    setYaxis(d_ymin, d_ymax);
}

void FrequencyDisplayPlot::setAutoScale(bool state) { d_autoscale_state = state; }

void FrequencyDisplayPlot::setAutoScaleShot()
{
    d_autoscale_state = true;
    d_autoscale_shot = true;
}

void FrequencyDisplayPlot::setPlotPosHalf(bool half)
{
    d_half_freq = half;
    if (half)
        d_start_frequency = d_center_frequency;
}


void FrequencyDisplayPlot::setMaxFFTVisible(const bool visibleFlag)
{
    d_max_fft_visible = visibleFlag;
    d_max_fft_plot_curve->setVisible(visibleFlag);
}

bool FrequencyDisplayPlot::getMaxFFTVisible() const { return d_max_fft_visible; }

void FrequencyDisplayPlot::setMinFFTVisible(const bool visibleFlag)
{
    d_min_fft_visible = visibleFlag;
    d_min_fft_plot_curve->setVisible(visibleFlag);
}

bool FrequencyDisplayPlot::getMinFFTVisible() const { return d_min_fft_visible; }

void FrequencyDisplayPlot::_resetXAxisPoints()
{
    double fft_bin_size =
        (d_stop_frequency - d_start_frequency) / static_cast<double>(d_numPoints);
    double freqValue = d_start_frequency;
    for (int64_t loc = 0; loc < d_numPoints; loc++) {
        d_xdata[loc] = freqValue;
        freqValue += fft_bin_size;
    }

    setAxisScale(QwtPlot::xBottom, d_start_frequency, d_stop_frequency);

    // Set up zoomer base for maximum unzoom x-axis
    // and reset to maximum unzoom level
    QRectF zbase = d_zoomer->zoomBase();
    d_zoomer->zoom(zbase);
    d_zoomer->setZoomBase(zbase);
    d_zoomer->setZoomBase(true);
    d_zoomer->zoom(0);
}

void FrequencyDisplayPlot::setLowerIntensityLevel(const double lowerIntensityLevel)
{
    d_lower_intensity_marker->setYValue(lowerIntensityLevel);
}

void FrequencyDisplayPlot::setUpperIntensityLevel(const double upperIntensityLevel)
{
    d_upper_intensity_marker->setYValue(upperIntensityLevel);
}

void FrequencyDisplayPlot::setTraceColour(QColor c) { d_plot_curve[0]->setPen(QPen(c)); }

void FrequencyDisplayPlot::setBGColour(QColor c)
{
    QPalette palette;
    palette.setColor(canvas()->backgroundRole(), c);
    canvas()->setPalette(palette);
}

void FrequencyDisplayPlot::showCFMarker(const bool show)
{
    if (show)
        d_marker_cf->show();
    else
        d_marker_cf->hide();
}

void FrequencyDisplayPlot::onPickerPointSelected(const QPointF& p)
{
    QPointF point = p;
    // fprintf(stderr,"onPickerPointSelected %f %f %d\n", point.x(), point.y(),
    // d_xdata_multiplier);
    point.setX(point.x() * d_xdata_multiplier);
    emit plotPointSelected(point);
}

void FrequencyDisplayPlot::setYLabel(const std::string& label, const std::string& unit)
{
    std::string l = label;
    if (!unit.empty())
        l += " (" + unit + ")";
    setAxisTitle(QwtPlot::yLeft, QString(l.c_str()));
    static_cast<FreqDisplayZoomer*>(d_zoomer)->setYUnit(unit);
}

void FrequencyDisplayPlot::setMinFFTColor(QColor c)
{
    d_min_fft_color = c;
    d_min_fft_plot_curve->setPen(QPen(c));
}
const QColor FrequencyDisplayPlot::getMinFFTColor() const { return d_min_fft_color; }

void FrequencyDisplayPlot::setMaxFFTColor(QColor c)
{
    d_max_fft_color = c;
    d_max_fft_plot_curve->setPen(QPen(c));
}

const QColor FrequencyDisplayPlot::getMaxFFTColor() const { return d_max_fft_color; }

void FrequencyDisplayPlot::setMarkerLowerIntensityColor(QColor c)
{
    d_marker_lower_intensity_color = c;
    d_lower_intensity_marker->setLinePen(QPen(c));
}
const QColor FrequencyDisplayPlot::getMarkerLowerIntensityColor() const
{
    return d_marker_lower_intensity_color;
}

void FrequencyDisplayPlot::setMarkerLowerIntensityVisible(bool visible)
{
    d_marker_lower_intensity_visible = visible;
    if (visible)
        d_lower_intensity_marker->setLineStyle(QwtPlotMarker::HLine);
    else
        d_lower_intensity_marker->setLineStyle(QwtPlotMarker::NoLine);
}
bool FrequencyDisplayPlot::getMarkerLowerIntensityVisible() const
{
    return d_marker_lower_intensity_visible;
}

void FrequencyDisplayPlot::setMarkerUpperIntensityColor(QColor c)
{
    d_marker_upper_intensity_color = c;
    d_upper_intensity_marker->setLinePen(QPen(c, 0, Qt::DotLine));
}

const QColor FrequencyDisplayPlot::getMarkerUpperIntensityColor() const
{
    return d_marker_upper_intensity_color;
}

void FrequencyDisplayPlot::setMarkerUpperIntensityVisible(bool visible)
{
    d_marker_upper_intensity_visible = visible;
    if (visible)
        d_upper_intensity_marker->setLineStyle(QwtPlotMarker::HLine);
    else
        d_upper_intensity_marker->setLineStyle(QwtPlotMarker::NoLine);
}

bool FrequencyDisplayPlot::getMarkerUpperIntensityVisible() const
{
    return d_marker_upper_intensity_visible;
}

void FrequencyDisplayPlot::setMarkerPeakAmplitudeColor(QColor c)
{
    d_marker_peak_amplitude_color = c;
    d_marker_peak_amplitude->setLinePen(QPen(c));
    QwtSymbol symbol;
    symbol.setStyle(QwtSymbol::Diamond);
    symbol.setSize(8);
    symbol.setPen(QPen(c));
    symbol.setBrush(QBrush(c));
    d_marker_peak_amplitude->setSymbol(&symbol);
}
const QColor FrequencyDisplayPlot::getMarkerPeakAmplitudeColor() const
{
    return d_marker_peak_amplitude_color;
}

void FrequencyDisplayPlot::setMarkerNoiseFloorAmplitudeColor(QColor c)
{
    d_marker_noise_floor_amplitude_color = c;
    d_marker_noise_floor_amplitude->setLinePen(QPen(c, 0, Qt::DotLine));
}

const QColor FrequencyDisplayPlot::getMarkerNoiseFloorAmplitudeColor() const
{
    return d_marker_noise_floor_amplitude_color;
}

void FrequencyDisplayPlot::setMarkerNoiseFloorAmplitudeVisible(bool visible)
{
    d_marker_noise_floor_amplitude_visible = visible;
    if (visible)
        d_marker_noise_floor_amplitude->setLineStyle(QwtPlotMarker::HLine);
    else
        d_marker_noise_floor_amplitude->setLineStyle(QwtPlotMarker::NoLine);
}

bool FrequencyDisplayPlot::getMarkerNoiseFloorAmplitudeVisible() const
{
    return d_marker_noise_floor_amplitude_visible;
}

void FrequencyDisplayPlot::setMarkerCFColor(QColor c)
{
    d_marker_cf_color = c;
    d_marker_cf->setLinePen(QPen(c, 0, Qt::DotLine));
}

const QColor FrequencyDisplayPlot::getMarkerCFColor() const { return d_marker_cf_color; }

void FrequencyDisplayPlot::attachTriggerLine(bool en)
{
    if (en) {
        d_trigger_line->attach(this);
    } else {
        d_trigger_line->detach();
    }
}

void FrequencyDisplayPlot::setTriggerLine(double level)
{
    d_trigger_line->setYValue(level);
}

#endif /* FREQUENCY_DISPLAY_PLOT_C */
