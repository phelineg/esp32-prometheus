#ifndef METRIC_H
#define METRIC_H

#include <Arduino.h>
#include <map>

enum MetricType {
    GAUGE,
    COUNTER,
    HISTOGRAM,
    SUMMARY
};

String metricTypeToString(MetricType type);

std::map<String, String> extendLabels(const std::map<String, String>& baseLabels, const std::map<String, String>& additionalLabels);

String createPrometheusMetricString(
    const String& name, 
    const String& description, 
    MetricType type, 
    const std::map<String, String>& labels, 
    const String& value
);

#endif
