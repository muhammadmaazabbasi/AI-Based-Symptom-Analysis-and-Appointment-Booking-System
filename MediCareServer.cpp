#include "MediCareServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <regex>
#include <fstream>

namespace MediCare {

// Doct class
bool Doctor::hasSpecialization(const std::string& spec) const {
    for (const auto& s : specializations) {
        if (s.find(spec) != std::string::npos || spec.find(s) != std::string::npos) {
            return true;
        }
    }
    return specialty.find(spec) != std::string::npos || spec.find(specialty) != std::string::npos;
}

std::string Doctor::toHtmlCard(bool isRecommended) const {
    std::ostringstream html;
    std::string fee = std::to_string(consultationFee / 100);
    std::string recommendedClass = isRecommended ? " style='border: 3px solid #2563eb; background: linear-gradient(135deg, #eff6ff, #f0f9ff);'" : "";
    std::string recommendedBadge = isRecommended ? "  RECOMMENDED" : "";
    
    html << "<div class='doctor-card'" << recommendedClass << ">\n";
    html << "  <div class='doctor-header'>\n";
    html << "    <img src='" << imageUrl << "' alt='" << name << "' class='doctor-avatar'>\n";
    html << "    <div class='doctor-info'>\n";
    html << "      <h3>" << name << recommendedBadge << "</h3>\n";
    html << "      <div class='doctor-specialty'>" << specialty << "</div>\n";
    html << "      <div class='doctor-stats'>\n";
    html << "        <span> " << rating << " (" << reviewCount << " reviews)</span>\n";
    html << "        <span> " << experience << " years experience</span>\n";
    html << "      </div>\n";
    html << "    </div>\n";
    html << "  </div>\n";
    html << "  <div class='doctor-bio'>" << bio << "</div>\n";
    html << "  <div class='doctor-footer'>\n";
    html << "    <div class='doctor-fee'>PKR" << fee << " consultation</div>\n";
    html << "    <form action='/book' method='POST' style='display: inline;'>\n";
    html << "      <input type='hidden' name='doctor_id' value='" << id << "'>\n";
    html << "      <button type='submit' class='book-btn'> Book Appointment</button>\n";
    html << "    </form>\n";
    html << "  </div>\n";
    html << "</div>\n";
    
    return html.str();
}

// symtomanalysi implementation
void SymptomAnalysis::addCondition(const std::string& condition, const std::string& description, int confidence) {
    possibleConditions.emplace_back(condition, description, confidence);
}

void SymptomAnalysis::addRecommendation(const std::string& recommendation) {
    recommendations.push_back(recommendation);
}

void SymptomAnalysis::addWarningSign(const std::string& warning) {
    warningSignsWarnings.push_back(warning);
}

void SymptomAnalysis::addSuggestedSpecialty(const std::string& specialty) {
    suggestedSpecialties.push_back(specialty);
}

std::string SymptomAnalysis::toHtmlResults(bool showRaw) const {
    std::ostringstream html;
    // Show extracted main AI text (if available)
    if (!mainAIText.empty()) {
        html << "<div style='background:#f0f9ff; border:1.5px solid #2563eb; border-radius:12px; padding:18px; margin-bottom:22px;'>";
        html << "<b>AI Main Response:</b><br>";
        std::string cleaned = mainAIText;
        // Headings
        size_t pos = 0;
        while ((pos = cleaned.find("## ", pos)) != std::string::npos) {
            cleaned.replace(pos, 3, "<strong style='font-size:1.1em;'>");
            size_t end = cleaned.find('\n', pos);
            if (end != std::string::npos) {
                cleaned.insert(end, "</strong>");
                pos = end + 10;
            } else {
                cleaned.append("</strong>");
                break;
            }
        }
        // Bold
        pos = 0;
        while ((pos = cleaned.find("**", pos)) != std::string::npos) {
            size_t end = cleaned.find("**", pos + 2);
            if (end != std::string::npos) {
                cleaned.replace(end, 2, "</b>");
                cleaned.replace(pos, 2, "<b>");
                pos = end + 3;
            } else {
                break;
            }
        }
        // Bullet lists: wrap with <ul> ... </ul>
        bool inList = false;
        pos = 0;
        while ((pos = cleaned.find("<li>", pos)) != std::string::npos) {
            if (!inList) {
                cleaned.insert(pos, "<ul style='margin:0 0 0 18px;'>");
                inList = true;
                pos += 31; // length of <ul style='margin:0 0 0 18px;'>
            }
            size_t endLi = cleaned.find("</li>", pos);
            if (endLi != std::string::npos) {
                pos = endLi + 5;
                // Check if next is not <li>, close the list
                size_t nextLi = cleaned.find("<li>", pos);
                if (nextLi == std::string::npos || nextLi > cleaned.find("<br>", pos)) {
                    cleaned.insert(pos, "</ul>");
                    inList = false;
                }
            } else {
                break;
            }
        }
        // Replace * bullet points with <li>
        pos = 0;
        while ((pos = cleaned.find("\n* ", pos)) != std::string::npos) {
            cleaned.replace(pos, 3, "<li>");
            size_t end = cleaned.find('\n', pos + 4);
            if (end != std::string::npos) {
                cleaned.insert(end, "</li>");
                pos = end + 5;
            } else {
                cleaned.append("</li>");
                break;
            }
        }
        // Paragraphs
        while ((pos = cleaned.find("\n\n")) != std::string::npos) {
            cleaned.replace(pos, 2, "<br><br>");
        }
        // Line breaks
        while ((pos = cleaned.find("\n")) != std::string::npos) {
            cleaned.replace(pos, 1, "<br>");
        }
        html << "<div style='font-size:15px; color:#334155; background:none; border:none; margin:0; padding:0;'>" << cleaned << "</div></div>\n";
    }
    // Dropdown for AI raw response
    html << "<details style='margin-bottom: 25px;'>\n";
    html << "  <summary style='font-weight:600; font-size:1.1rem; color:#2563eb; cursor:pointer;'>Show AI Raw Response (for debugging)</summary>\n";
    html << "  <pre style='background:#f3f4f6; color:#334155; border:1px solid #e5e7eb; border-radius:10px; padding:18px; margin-top:12px; max-height:300px; overflow:auto; font-size:13px;'>";
    html << (rawAIResponse.empty() ? "<i>No raw response available.</i>" : rawAIResponse) << "</pre>\n";
    html << "</details>\n";
    
    html << "<div class='analysis-results'>\n";
    html << "  <h3>🔍 Possible Conditions</h3>\n";
    
    for (const auto& condition : possibleConditions) {
        std::string confidenceClass = condition.confidence >= 70 ? "high-confidence" : 
                                    condition.confidence >= 50 ? "medium-confidence" : "low-confidence";
        std::string badgeClass = condition.confidence >= 70 ? "confidence-high" : 
                               condition.confidence >= 50 ? "confidence-medium" : "confidence-low";
        
        html << "  <div class='condition " << confidenceClass << "'>\n";
        html << "    <div>\n";
        html << "      <strong>" << condition.condition << "</strong>\n";
        html << "      <p style='margin: 5px 0; color: #64748b;'>" << condition.description << "</p>\n";
        html << "    </div>\n";
        html << "    <span class='confidence-badge " << badgeClass << "'>" << condition.confidence << "%</span>\n";
        html << "  </div>\n";
    }
    
    html << "  <h3 style='margin-top: 30px;'> AI Recommendations</h3>\n";
    html << "  <ul class='recommendations'>\n";
    for (const auto& rec : recommendations) {
        html << "    <li>" << rec << "</li>\n";
    }
    html << "  </ul>\n";
    
    html << "  <h3 style='margin-top: 30px;'> Seek Immediate Care If:</h3>\n";
    html << "  <ul class='warnings'>\n";
    for (const auto& warning : warningSignsWarnings) {
        html << "    <li>" << warning << "</li>\n";
    }
    html << "  </ul>\n";
    html << "</div>\n";
    
    return html.str();
}

// AIService implementation
AIService::AIService(const std::string& key) : apiKey(key) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
}

AIService::~AIService() {
    if (curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

size_t AIService::WriteCallbackFunction(void* contents, size_t size, size_t nmemb, WriteCallback* userp) {
    size_t totalSize = size * nmemb;
    userp->data.append((char*)contents, totalSize);
    return totalSize;
}

std::string AIService::makeHttpRequest(const std::string& url, const std::string& payload) {
    if (!curl) return "";
    
    WriteCallback writeCallback;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallbackFunction);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeCallback);
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    
    return (res == CURLE_OK) ? writeCallback.data : "";
}

// Utility: Extract the first "text" field value from Gemini AP
static std::string extractMainAIText(const std::string& raw) {
    const std::string key = "\"text\":";
    size_t pos = raw.find(key);
    if (pos == std::string::npos) return "";
    pos += key.length();
    // Skip whitespace and possible space after colon
    while (pos < raw.size() && (raw[pos] == ' ' || raw[pos] == '"')) ++pos;
    size_t end = pos;
    bool inEscape = false;
    std::string result;
    // Extract until next unescaped quote
    while (end < raw.size()) {
        char c = raw[end];
        if (c == '\\' && !inEscape) {
            inEscape = true;
        } else if (c == '"' && !inEscape) {
            break;
        } else {
            if (inEscape && c != '"' && c != '\\') result += '\\';
            result += c;
            inEscape = false;
        }
        ++end;
    }
    return result;
}

std::unique_ptr<SymptomAnalysis> AIService::analyzeSymptoms(const std::string& symptoms, 
                                                          const std::string& duration, 
                                                          int severity) {
    auto analysis = std::make_unique<SymptomAnalysis>(symptoms, duration, severity);
    
    // Create Gemini API request payload
    std::ostringstream payload;
    payload << "{\n";
    payload << "  \"contents\": [{\n";
    payload << "    \"parts\": [{\n";
    payload << "      \"text\": \"As a medical AI assistant, analyze these symptoms:\\n\\n";
    payload << "Symptoms: " << symptoms << "\\n";
    payload << "Duration: " << (duration.empty() ? "Not specified" : duration) << "\\n";
    payload << "Severity (1-10): " << severity << "\\n\\n";
    payload << "Provide analysis with possible conditions, recommendations, warning signs, and suggested specialties.\"\n";
    payload << "    }]\n";
    payload << "  }]\n";
    payload << "}";
    
    std::string url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-1.5-flash-latest:generateContent?key=" + apiKey;
    std::string response = makeHttpRequest(url, payload.str());
    analysis->setRawAIResponse(response); // Store the raw AI response
    analysis->setMainAIText(extractMainAIText(response)); // Store the extracted main AI text
    
    // Parse response and extract medical insights
    if (response.find("respiratory") != std::string::npos || 
        symptoms.find("cough") != std::string::npos || 
        symptoms.find("breathing") != std::string::npos) {
        analysis->addCondition("Respiratory Infection", "Possible viral or bacterial respiratory infection", 75);
        analysis->addSuggestedSpecialty("Pulmonology");
        analysis->addSuggestedSpecialty("Internal Medicine");
    }
    
    if (symptoms.find("headache") != std::string::npos || 
        symptoms.find("head") != std::string::npos) {
        analysis->addCondition("Tension Headache", "Common type of headache caused by stress or muscle tension", 80);
        analysis->addSuggestedSpecialty("Neurology");
        analysis->addSuggestedSpecialty("Family Medicine");
    }
    
    if (symptoms.find("chest") != std::string::npos || 
        symptoms.find("heart") != std::string::npos) {
        analysis->addCondition("Chest Discomfort", "Could be related to cardiac or respiratory issues", 70);
        analysis->addSuggestedSpecialty("Cardiology");
        analysis->addSuggestedSpecialty("Internal Medicine");
    }
    
    if (symptoms.find("fever") != std::string::npos || 
        symptoms.find("temperature") != std::string::npos) {
        analysis->addCondition("Viral Infection", "Common viral illness with fever symptoms", 85);
        analysis->addSuggestedSpecialty("Internal Medicine");
        analysis->addSuggestedSpecialty("Family Medicine");
    }
    
    // Add general recommendations
    analysis->addRecommendation("Consult with a healthcare professional for proper diagnosis");
    analysis->addRecommendation("Monitor symptoms closely and note any changes");
    analysis->addRecommendation("Rest and stay hydrated");
    analysis->addRecommendation("Take over-the-counter medication if needed for symptom relief");
    
    // Add warning signs
    analysis->addWarningSign("Difficulty breathing or shortness of breath");
    analysis->addWarningSign("Severe or worsening pain");
    analysis->addWarningSign("High fever above 102°F");
    analysis->addWarningSign("Loss of consciousness or confusion");
    
    // Default fallback if no specific conditions were added
    if (analysis->getSuggestedSpecialties().empty()) {
        analysis->addCondition("General Medical Assessment", "Symptoms require professional medical evaluation", 75);
        analysis->addSuggestedSpecialty("Internal Medicine");
        analysis->addSuggestedSpecialty("Family Medicine");
    }
    
    return analysis;
}

// HttpServer implementation
HttpServer::HttpServer(int port, const std::string& geminiApiKey) 
    : port(port), running(false) {
    initializeDoctors();
    aiService = std::make_unique<AIService>(geminiApiKey);
}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::initializeDoctors() {
    doctors.clear();
    doctors.push_back(std::make_shared<Doctor>(
        1, "Dr. Abdul Rehman", "Internal Medicine", 15, 4.9, 127,
        "Specializes in respiratory infections, general internal medicine, and preventive care. Excellent track record with viral infections and symptom management.",
        12000, "https://images.unsplash.com/photo-1612349317150-e413f6a5b16d?ixlib=rb-4.0.3&auto=format&fit=crop&w=120&h=120",
        std::vector<std::string>{"Respiratory Care", "Internal Medicine", "Preventive Care"}
    ));
    doctors.push_back(std::make_shared<Doctor>(
        2, "Dr. SARA", "Family Medicine", 12, 4.7, 89,
        "Comprehensive family medicine with focus on holistic care and patient education. Experienced in treating common illnesses and wellness management.",
        10000, "https://images.unsplash.com/photo-1559839734-2b71ea197ec2?ixlib=rb-4.0.3&auto=format&fit=crop&w=120&h=120",
        std::vector<std::string>{"Family Medicine", "Wellness Care"}
    ));
    doctors.push_back(std::make_shared<Doctor>(
        3, "Dr. Ahmed", "Pulmonology", 20, 4.8, 156,
        "Specialist in lung and respiratory system disorders. Expert in treating breathing difficulties, chronic cough, and respiratory infections.",
        15000, "https://images.unsplash.com/photo-1582750433449-648ed127bb54?ixlib=rb-4.0.3&auto=format&fit=crop&w=120&h=120",
        std::vector<std::string>{"Pulmonology", "Respiratory Care"}
    ));
    doctors.push_back(std::make_shared<Doctor>(
        4, "Dr. haris", "Cardiology", 18, 4.9, 203,
        "Heart specialist with expertise in cardiovascular diseases, chest pain evaluation, and cardiac preventive care.",
        18000, "https://images.unsplash.com/photo-1594824694996-639a8b70a788?ixlib=rb-4.0.3&auto=format&fit=crop&w=120&h=120",
        std::vector<std::string>{"Cardiology", "Chest Pain", "Heart Disease"}
    ));
    doctors.push_back(std::make_shared<Doctor>(
        5, "Dr. Mahad", "Neurology", 16, 4.6, 94,
        "Neurologist specializing in headaches, migraines, and neurological disorders. Expert in brain and nervous system conditions.",
        16000, "https://images.unsplash.com/photo-1607990281513-2c110a25bd8c?ixlib=rb-4.0.3&auto=format&fit=crop&w=120&h=120",
        std::vector<std::string>{"Neurology", "Headaches", "Migraines"}
    ));
}

std::shared_ptr<Doctor> HttpServer::getDoctorById(int id) const {
    for (const auto& doctor : doctors) {
        if (doctor->getId() == id) {
            return doctor;
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<Doctor>> HttpServer::getDoctorsBySpecialty(const std::string& specialty) const {
    std::vector<std::shared_ptr<Doctor>> filtered;
    for (const auto& doctor : doctors) {
        if (doctor->hasSpecialization(specialty)) {
            filtered.push_back(doctor);
        }
    }
    return filtered;
}

void HttpServer::writeAppointmentToFile(const std::string& details) {
    std::ofstream file("appointments.txt", std::ios::app);
    if (file.is_open()) {
        file << details << "\n";
        file.close();
    }
}

std::string HttpServer::parseFormData(const std::string& body) {
    return body;
}

std::string HttpServer::getFormValue(const std::string& formData, const std::string& key) {
    std::string searchKey = key + "=";
    size_t pos = formData.find(searchKey);
    if (pos == std::string::npos) {
        return "";
    }
    
    size_t start = pos + searchKey.length();
    size_t end = formData.find("&", start);
    if (end == std::string::npos) {
        end = formData.length();
    }
    
    return urlDecode(formData.substr(start, end - start));
}

std::string HttpServer::urlDecode(const std::string& encoded) {
    std::string decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            int value = std::stoi(encoded.substr(i + 1, 2), nullptr, 16);
            decoded += static_cast<char>(value);
            i += 2;
        } else if (encoded[i] == '+') {
            decoded += ' ';
        } else {
            decoded += encoded[i];
        }
    }
    return decoded;
}

std::string HttpServer::handleHomePage() {
    std::ifstream file("index.html");
    if (file.is_open()) {
        std::ostringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    return "<h1>MediCare AI</h1><p>Index file not found</p>";
}

std::string HttpServer::handleAnalyzeSymptoms(const std::string& requestBody) {
    std::string symptoms = getFormValue(requestBody, "symptoms");
    std::string duration = getFormValue(requestBody, "duration");
    std::string severityStr = getFormValue(requestBody, "severity");
    
    int severity = severityStr.empty() ? 5 : std::stoi(severityStr);
    
    // Get AI analysis
    auto analysis = aiService->analyzeSymptoms(symptoms, duration, severity);
    
    // Get recommended doctors
    std::vector<std::shared_ptr<Doctor>> recommendedDoctors;
    for (const auto& specialty : analysis->getSuggestedSpecialties()) {
        auto doctors = getDoctorsBySpecialty(specialty);
        for (const auto& doctor : doctors) {
            // Avoid duplicates
            bool found = false;
            for (const auto& existing : recommendedDoctors) {
                if (existing->getId() == doctor->getId()) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                recommendedDoctors.push_back(doctor);
            }
        }
    }
    
    // Limit to top 3 doctors
    if (recommendedDoctors.size() > 3) {
        recommendedDoctors.resize(3);
    }
    
    // Generate HTML response
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html><head><title>Analysis Results - MediCare AI</title>\n";
    html << "<style>\n";
    html << "body { font-family: 'Segoe UI', sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); margin: 0; padding: 20px; }\n";
    html << ".container { max-width: 1200px; margin: 0 auto; }\n";
    html << ".card { background: rgba(255,255,255,0.95); border-radius: 20px; padding: 40px; margin-bottom: 30px; box-shadow: 0 20px 40px rgba(0,0,0,0.1); }\n";
    html << ".condition { background: #f8fafc; border: 2px solid #e2e8f0; border-radius: 10px; padding: 15px; margin-bottom: 15px; display: flex; justify-content: space-between; align-items: center; }\n";
    html << ".condition.high-confidence { border-color: #ef4444; background: #fef2f2; }\n";
    html << ".condition.medium-confidence { border-color: #f59e0b; background: #fffbeb; }\n";
    html << ".condition.low-confidence { border-color: #3b82f6; background: #eff6ff; }\n";
    html << ".confidence-badge { padding: 4px 12px; border-radius: 20px; font-weight: 600; font-size: 14px; }\n";
    html << ".confidence-high { background: #ef4444; color: white; }\n";
    html << ".confidence-medium { background: #f59e0b; color: white; }\n";
    html << ".confidence-low { background: #3b82f6; color: white; }\n";
    html << ".recommendations, .warnings { list-style: none; margin: 15px 0; }\n";
    html << ".recommendations li, .warnings li { padding: 10px 0; border-bottom: 1px solid #e5e7eb; position: relative; padding-left: 25px; }\n";
    html << ".recommendations li:before { content: '✓'; position: absolute; left: 0; color: #059669; font-weight: bold; }\n";
    html << ".warnings li:before { content: '⚠'; position: absolute; left: 0; color: #ef4444; }\n";
    html << ".doctor-card { background: white; border: 2px solid #e5e7eb; border-radius: 15px; padding: 20px; margin-bottom: 20px; }\n";
    html << ".doctor-header { display: flex; align-items: center; gap: 15px; margin-bottom: 15px; }\n";
    html << ".doctor-avatar { width: 60px; height: 60px; border-radius: 50%; object-fit: cover; }\n";
    html << ".doctor-specialty { color: #2563eb; font-weight: 600; }\n";
    html << ".doctor-stats { display: flex; gap: 15px; font-size: 14px; color: #64748b; margin-top: 5px; }\n";
    html << ".doctor-bio { margin: 15px 0; color: #64748b; line-height: 1.5; }\n";
    html << ".doctor-footer { display: flex; justify-content: space-between; align-items: center; margin-top: 15px; }\n";
    html << ".doctor-fee { font-size: 1.2rem; font-weight: 600; color: #1e293b; }\n";
    html << ".book-btn { background: #2563eb; color: white; border: none; padding: 10px 20px; border-radius: 8px; cursor: pointer; font-weight: 600; }\n";
    html << ".btn { background: linear-gradient(45deg, #2563eb, #7c3aed); color: white; border: none; padding: 15px 30px; border-radius: 12px; font-size: 16px; font-weight: 600; cursor: pointer; text-decoration: none; display: inline-block; }\n";
    html << "details[open] summary { color: #7c3aed; }\n";
    html << "details summary { outline: none; }\n";
    html << "</style></head><body>\n";
    html << "<div class='container'>\n";
    html << "<div class='card'>\n";
    html << "<h1> AI Analysis Results</h1>\n";
    html << analysis->toHtmlResults(true);
    html << "</div>\n";
    
    if (!recommendedDoctors.empty()) {
        html << "<div class='card'>\n";
        html << "<h2> Recommended Doctors</h2>\n";
        html << "<p>Based on your symptoms, these specialists are best suited to help you.</p>\n";
        
        for (size_t i = 0; i < recommendedDoctors.size(); ++i) {
            html << recommendedDoctors[i]->toHtmlCard(i == 0);
        }
        html << "</div>\n";
    }
    
    html << "<div class='card'>\n";
    html << "<a href='/' class='btn'> New Analysis</a>\n";
    html << "</div>\n";
    html << "</div></body></html>";
    
    return html.str();
}

std::string HttpServer::handleBookAppointment(const std::string& requestBody) {
    std::string doctorIdStr = getFormValue(requestBody, "doctor_id");
    int doctorId = std::stoi(doctorIdStr);
    
    auto doctor = getDoctorById(doctorId);
    if (!doctor) {
        return createHttpResponse(404, "<h1>Doctor not found</h1>");
    }
    
    // If this is a booking confirmation (POST to /confirm-booking), write appointment to file
    // Otherwise, show the booking form
    if (requestBody.find("patient_name=") != std::string::npos) {
        std::string patientName = getFormValue(requestBody, "patient_name");
        std::string patientEmail = getFormValue(requestBody, "patient_email");
        std::string patientPhone = getFormValue(requestBody, "patient_phone");
        std::string appointmentDate = getFormValue(requestBody, "appointment_date");
        std::string appointmentTime = getFormValue(requestBody, "appointment_time");
        std::string appointmentType = getFormValue(requestBody, "appointment_type");
        std::string notes = getFormValue(requestBody, "notes");
        std::ostringstream details;
        details << "Doctor: " << doctor->getName() << " (" << doctor->getSpecialty() << ")\n";
        details << "Patient: " << patientName << "\n";
        details << "Email: " << patientEmail << "\n";
        details << "Phone: " << patientPhone << "\n";
        details << "Date: " << appointmentDate << "\n";
        details << "Time: " << appointmentTime << "\n";
        details << "Type: " << appointmentType << "\n";
        details << "Notes: " << notes << "\n";
        details << "-----------------------------";
        writeAppointmentToFile(details.str());
        return createHttpResponse(200, "<html><body><h1> Appointment Booked Successfully!</h1><p>You will receive a confirmation email shortly.</p><a href='/'>← Back to Home</a></body></html>");
    }
    
    // Generate booking form HTML
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html><head><title>Book Appointment - MediCare AI</title>\n";
    html << "<style>\n";
    html << "body { font-family: 'Segoe UI', sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); margin: 0; padding: 20px; }\n";
    html << ".container { max-width: 800px; margin: 0 auto; }\n";
    html << ".card { background: rgba(255,255,255,0.95); border-radius: 20px; padding: 40px; margin-bottom: 30px; box-shadow: 0 20px 40px rgba(0,0,0,0.1); }\n";
    html << ".form-group { margin-bottom: 25px; }\n";
    html << "label { display: block; margin-bottom: 8px; font-weight: 600; color: #374151; }\n";
    html << "input, select, textarea { width: 100%; padding: 12px 16px; border: 2px solid #e5e7eb; border-radius: 10px; font-size: 16px; }\n";
    html << ".btn { background: linear-gradient(45deg, #2563eb, #7c3aed); color: white; border: none; padding: 15px 30px; border-radius: 12px; font-size: 16px; font-weight: 600; cursor: pointer; width: 100%; }\n";
    html << "</style></head><body>\n";
    
    html << "<div class='container'>\n";
    html << "<div class='card'>\n";
    html << "<h1> Book Appointment with " << doctor->getName() << "</h1>\n";
    html << "<p><strong>Specialty:</strong> " << doctor->getSpecialty() << "</p>\n";
    html << "<p><strong>Consultation Fee:</strong> PKR" << (doctor->getConsultationFee() / 100) << "</p>\n";
    html << "<br>\n";
    
    html << "<form action='/book' method='POST'>\n";
    html << "<input type='hidden' name='doctor_id' value='" << doctorId << "'>\n";
    
    html << "<div class='form-group'>\n";
    html << "<label>Full Name *</label>\n";
    html << "<input type='text' name='patient_name' required>\n";
    html << "</div>\n";
    
    html << "<div class='form-group'>\n";
    html << "<label>Email Address *</label>\n";
    html << "<input type='email' name='patient_email' required>\n";
    html << "</div>\n";
    
    html << "<div class='form-group'>\n";
    html << "<label>Phone Number *</label>\n";
    html << "<input type='tel' name='patient_phone' required>\n";
    html << "</div>\n";
    
    html << "<div class='form-group'>\n";
    html << "<label>Preferred Date *</label>\n";
    html << "<input type='date' name='appointment_date' required>\n";
    html << "</div>\n";
    
    html << "<div class='form-group'>\n";
    html << "<label>Preferred Time *</label>\n";
    html << "<select name='appointment_time' required>\n";
    html << "<option value=''>Select time</option>\n";
    html << "<option value='9:00 AM'>9:00 AM</option>\n";
    html << "<option value='10:00 AM'>10:00 AM</option>\n";
    html << "<option value='11:00 AM'>11:00 AM</option>\n";
    html << "<option value='2:00 PM'>2:00 PM</option>\n";
    html << "<option value='3:00 PM'>3:00 PM</option>\n";
    html << "<option value='4:00 PM'>4:00 PM</option>\n";
    html << "</select>\n";
    html << "</div>\n";
    
    html << "<div class='form-group'>\n";
    html << "<label>Appointment Type *</label>\n";
    html << "<select name='appointment_type' required>\n";
    html << "<option value='in-person'>In-Person Visit</option>\n";
    html << "<option value='video'>Video Consultation</option>\n";
    html << "</select>\n";
    html << "</div>\n";
    
    html << "<div class='form-group'>\n";
    html << "<label>Additional Notes</label>\n";
    html << "<textarea name='notes' placeholder='Any specific concerns or information for the doctor...'></textarea>\n";
    html << "</div>\n";
    
    html << "<button type='submit' class='btn'> Confirm Appointment</button>\n";
    html << "</form>\n";
    
    html << "<br><a href='/' style='color: #2563eb;'>← Back to Home</a>\n";
    html << "</div>\n";
    html << "</div></body></html>";
    
    return html.str();
}

std::string HttpServer::createHttpResponse(int statusCode, const std::string& body, const std::string& contentType) {
    std::ostringstream response;
    response << "HTTP/1.1 " << statusCode << " OK\r\n";
    response << "Content-Type: " << contentType << "\r\n";
    response << "Content-Length: " << body.length() << "\r\n";
    response << "Connection: close\r\n";
    response << "\r\n";
    response << body;
    return response.str();
}

bool HttpServer::start() {
    running = true;
    return true;
}

void HttpServer::stop() {
    running = false;
}

void HttpServer::run() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(serverSocket);
        return;
    }
    
    if (listen(serverSocket, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(serverSocket);
        return;
    }
    
    std::cout << " MediCare AI Server running on port " << port << std::endl;
    std::cout << " Visit: http://localhost:" << port << std::endl;
    
    while (running) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket < 0) continue;
        
        char buffer[4096] = {0};
        read(clientSocket, buffer, sizeof(buffer));
        
        std::string request(buffer);
        std::string response;
        
        // Parse HTTP request
        std::istringstream requestStream(request);
        std::string method, path, version;
        requestStream >> method >> path >> version;
        
        // Route handling
        if (method == "GET" && path == "/") {
            response = createHttpResponse(200, handleHomePage());
        } else if (method == "POST" && path == "/analyze") {
            size_t bodyStart = request.find("\r\n\r\n");
            std::string body = (bodyStart != std::string::npos) ? request.substr(bodyStart + 4) : "";
            response = createHttpResponse(200, handleAnalyzeSymptoms(body));
        } else if (method == "POST" && path == "/book") {
            size_t bodyStart = request.find("\r\n\r\n");
            std::string body = (bodyStart != std::string::npos) ? request.substr(bodyStart + 4) : "";
            response = createHttpResponse(200, handleBookAppointment(body));
        } else if (method == "POST" && path == "/confirm-booking") {
            response = createHttpResponse(200, "<html><body><h1> Appointment Booked Successfully!</h1><p>You will receive a confirmation email shortly.</p><a href='/'>← Back to Home</a></body></html>");
        } else {
            response = createHttpResponse(404, "<h1>404 - Page Not Found</h1>");
        }
        
        write(clientSocket, response.c_str(), response.length());
        close(clientSocket);
    }
    
    close(serverSocket);
}

} // namespace MediCare