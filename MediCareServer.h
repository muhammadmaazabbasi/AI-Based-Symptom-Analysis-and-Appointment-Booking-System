#ifndef MEDICARE_SERVER_H
#define MEDICARE_SERVER_H

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <curl/curl.h>

namespace MediCare {

// Doctor class with full OOP encapsulation
class Doctor {
private:
    int id;
    std::string name;
    std::string specialty;
    int experience;
    double rating;
    int reviewCount;
    std::string bio;
    int consultationFee;
    std::string imageUrl;
    std::vector<std::string> specializations;
    bool isAvailable;

public:
    // Constructor with initialization list
    Doctor(int id, const std::string& name, const std::string& specialty,
           int exp, double rat, int reviews, const std::string& bio,
           int fee, const std::string& img, const std::vector<std::string>& specs)
        : id(id), name(name), specialty(specialty), experience(exp), 
          rating(rat), reviewCount(reviews), bio(bio), consultationFee(fee),
          imageUrl(img), specializations(specs), isAvailable(true) {}

    // Getters (const methods for data integrity)
    int getId() const { return id; }
    std::string getName() const { return name; }
    std::string getSpecialty() const { return specialty; }
    int getExperience() const { return experience; }
    double getRating() const { return rating; }
    int getReviewCount() const { return reviewCount; }
    std::string getBio() const { return bio; }
    int getConsultationFee() const { return consultationFee; }
    std::string getImageUrl() const { return imageUrl; }
    std::vector<std::string> getSpecializations() const { return specializations; }
    bool getIsAvailable() const { return isAvailable; }

    // Polymorphic behavior for specialization matching
    virtual bool hasSpecialization(const std::string& spec) const;
    virtual std::string toHtmlCard(bool isRecommended = false) const;
    
    // Virtual destructor for proper inheritance
    virtual ~Doctor() = default;
};

// Appointment class with encapsulation
class Appointment {
private:
    int id;
    int doctorId;
    std::string patientName;
    std::string patientEmail;
    std::string patientPhone;
    std::string appointmentDate;
    std::string appointmentTime;
    std::string appointmentType;
    std::string symptoms;
    std::string notes;
    std::string status;

public:
    Appointment(int id, int docId, const std::string& name, const std::string& email,
                const std::string& phone, const std::string& date, const std::string& time,
                const std::string& type, const std::string& symp, const std::string& note)
        : id(id), doctorId(docId), patientName(name), patientEmail(email),
          patientPhone(phone), appointmentDate(date), appointmentTime(time),
          appointmentType(type), symptoms(symp), notes(note), status("scheduled") {}

    // Getters
    int getId() const { return id; }
    int getDoctorId() const { return doctorId; }
    std::string getPatientName() const { return patientName; }
    std::string getStatus() const { return status; }
    
    // Status management
    void setStatus(const std::string& newStatus) { status = newStatus; }
    
    virtual ~Appointment() = default;
};

// AI Analysis class with abstraction
class SymptomAnalysis {
public:
    struct Condition {
        std::string condition;
        std::string description;
        int confidence;
        
        Condition(const std::string& c, const std::string& d, int conf)
            : condition(c), description(d), confidence(conf) {}
    };

private:
    std::string symptoms;
    std::string duration;
    int severity;
    std::vector<Condition> possibleConditions;
    std::vector<std::string> recommendations;
    std::vector<std::string> warningSignsWarnings;
    std::vector<std::string> suggestedSpecialties;
    std::string rawAIResponse; // Store the raw AI response
    std::string mainAIText;    // Store the extracted main AI text

public:
    SymptomAnalysis(const std::string& symp, const std::string& dur, int sev)
        : symptoms(symp), duration(dur), severity(sev) {}

    // Data management methods
    void addCondition(const std::string& condition, const std::string& description, int confidence);
    void addRecommendation(const std::string& recommendation);
    void addWarningSign(const std::string& warning);
    void addSuggestedSpecialty(const std::string& specialty);

    // Getters
    std::vector<std::string> getSuggestedSpecialties() const { return suggestedSpecialties; }
    void setRawAIResponse(const std::string& raw) { rawAIResponse = raw; }
    std::string getRawAIResponse() const { return rawAIResponse; }
    void setMainAIText(const std::string& text) { mainAIText = text; }
    std::string getMainAIText() const { return mainAIText; }

    // HTML generation for display
    std::string toHtmlResults(bool showRaw = false) const;
    
    virtual ~SymptomAnalysis() = default;
};

// AI Service with inheritance and polymorphism
class AIService {
private:
    std::string apiKey;
    CURL* curl;
    
    struct WriteCallback {
        std::string data;
    };
    
    static size_t WriteCallbackFunction(void* contents, size_t size, size_t nmemb, WriteCallback* userp);
    std::string makeHttpRequest(const std::string& url, const std::string& payload);

public:
    AIService(const std::string& key);
    virtual ~AIService();
    
    // Pure virtual method for analysis (can be overridden for different AI services)
    virtual std::unique_ptr<SymptomAnalysis> analyzeSymptoms(const std::string& symptoms, 
                                                            const std::string& duration, 
                                                            int severity);
};

// HTTP Server with composition and abstraction
class HttpServer {
private:
    int port;
    std::vector<std::shared_ptr<Doctor>> doctors; // Now owned directly
    std::unique_ptr<AIService> aiService;
    bool running;
    
    // Private methods for request handling
    std::string parseFormData(const std::string& body);
    std::string getFormValue(const std::string& formData, const std::string& key);
    std::string urlDecode(const std::string& encoded);
    
    // Route handlers
    std::string handleHomePage();
    std::string handleAnalyzeSymptoms(const std::string& requestBody);
    std::string handleBookAppointment(const std::string& requestBody);
    std::string createHttpResponse(int statusCode, const std::string& body, const std::string& contentType = "text/html");
    
    // Doctor management
    void initializeDoctors();
    std::shared_ptr<Doctor> getDoctorById(int id) const;
    std::vector<std::shared_ptr<Doctor>> getDoctorsBySpecialty(const std::string& specialty) const;

    // Appointment file writing
    void writeAppointmentToFile(const std::string& details);

public:
    HttpServer(int port, const std::string& geminiApiKey);
    virtual ~HttpServer();
    
    // Server lifecycle management
    bool start();
    void stop();
    bool isRunning() const { return running; }
    
    // Main server loop
    void run();
};

} // namespace MediCare

#endif // MEDICARE_SERVER_H