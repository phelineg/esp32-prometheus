#include <metric.h>

String metricTypeToString(MetricType type) {
    switch (type) {
        case GAUGE: return "gauge";
        case COUNTER: return "counter";
        case HISTOGRAM: return "histogram";
        case SUMMARY: return "summary";
        default: return "unknown";
    }
}

std::map<String, String> extendLabels(const std::map<String, String>& baseLabels, const std::map<String, String>& additionalLabels) {
    std::map<String, String> result = baseLabels;
    for (const auto& label : additionalLabels) {
        result[label.first] = label.second;
    }
    return result;
}

String createPrometheusMetricString(
    const String& name, 
    const String& description, 
    MetricType type, 
    const std::map<String, String>& labels, 
    const String& value
) {
    String result;
    result += "# HELP " + name + " " + description + "\n";
    result += "# TYPE " + name + " " + metricTypeToString(type) + "\n";

    String labelString;
    for (const auto& label : labels) {
        if (!labelString.isEmpty()) {
            labelString += ",";
        }
        labelString += label.first + "=\"" + label.second + "\"";
    }

    result += name + "{" + labelString + "} " + value + "\n";
    return result;
}