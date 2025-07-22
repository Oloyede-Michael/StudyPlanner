#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>
#include <memory>
#include <cmath>

using namespace std;

// Forward declarations
class Course;
class TimeSlot;
class StudySession;
class Schedule;

// ==================== BASE CLASSES (ABSTRACTION) ====================
class Schedulable {
public:
    virtual ~Schedulable() = default;
    virtual void displayInfo() const = 0;
    virtual double getPriority() const = 0;
};

class Persistable {
public:
    virtual ~Persistable() = default;
    virtual string serialize() const = 0;
    virtual void deserialize(const string& data) = 0;
};

// ==================== COURSE CLASS (ENCAPSULATION) ====================
class Course : public Schedulable, public Persistable {
private:
    string name;
    int difficulty;           // 1-5 scale
    string examDate;         // YYYY-MM-DD format
    int totalHoursNeeded;
    int hoursCompleted;
    double priority;
    
public:
    // Constructor
    Course(const string& courseName, int diff, const string& examDt, int totalHrs)
        : name(courseName), difficulty(diff), examDate(examDt), 
          totalHoursNeeded(totalHrs), hoursCompleted(0), priority(0.0) {
        calculatePriority();
    }
    
    // Getters (Encapsulation)
    string getName() const { return name; }
    int getDifficulty() const { return difficulty; }
    string getExamDate() const { return examDate; }
    int getTotalHours() const { return totalHoursNeeded; }
    int getCompletedHours() const { return hoursCompleted; }
    int getRemainingHours() const { return totalHoursNeeded - hoursCompleted; }
    
    // Setters with validation
    void setHoursCompleted(int hours) {
        if (hours >= 0 && hours <= totalHoursNeeded) {
            hoursCompleted = hours;
            calculatePriority();
        }
    }
    
    void addStudyHours(int hours) {
        if (hoursCompleted + hours <= totalHoursNeeded) {
            hoursCompleted += hours;
            calculatePriority();
        }
    }
    
    // FIXED: Calculate days until exam - corrected version
    int getDaysUntilExam() const {
        // Get current date
        time_t now = time(0);
        tm* timeinfo = localtime(&now);
        
        // Parse exam date (YYYY-MM-DD)
        int examYear = stoi(examDate.substr(0, 4));
        int examMonth = stoi(examDate.substr(5, 2));
        int examDay = stoi(examDate.substr(8, 2));
        
        // Get current date components
        int currentDay = timeinfo->tm_mday;
        int currentMonth = timeinfo->tm_mon + 1;  // tm_mon is 0-based
        int currentYear = timeinfo->tm_year + 1900;
        
        // Create tm structures for both dates at midnight (normalize time)
        tm examDate_tm = {};
        examDate_tm.tm_year = examYear - 1900;    // tm_year is years since 1900
        examDate_tm.tm_mon = examMonth - 1;       // tm_mon is 0-based (0 = January)
        examDate_tm.tm_mday = examDay;
        examDate_tm.tm_hour = 0;                  // Set to start of day
        examDate_tm.tm_min = 0;
        examDate_tm.tm_sec = 0;
        examDate_tm.tm_isdst = -1;                // Let system determine DST
        
        tm currentDate_tm = {};
        currentDate_tm.tm_year = currentYear - 1900;
        currentDate_tm.tm_mon = currentMonth - 1;
        currentDate_tm.tm_mday = currentDay;
        currentDate_tm.tm_hour = 0;               // Set to start of day
        currentDate_tm.tm_min = 0;
        currentDate_tm.tm_sec = 0;
        currentDate_tm.tm_isdst = -1;             // Let system determine DST
        
        // Convert to time_t for calculation
        time_t examTime = mktime(&examDate_tm);
        time_t currentTime = mktime(&currentDate_tm);
        
        // Handle invalid dates
        if (examTime == -1 || currentTime == -1) {
            return 30; // Fallback for invalid dates
        }
        
        // Calculate difference in seconds, then convert to days
        double secondsDiff = difftime(examTime, currentTime);
        
        // Round to nearest day instead of truncating
        int daysDiff = (int)round(secondsDiff / (24.0 * 60.0 * 60.0));
        
        return daysDiff;
    }
    
    // IMPROVED: Calculate priority with better handling of edge cases
    void calculatePriority() {
        int daysLeft = getDaysUntilExam();
        double completionRatio = (double)hoursCompleted / totalHoursNeeded;
        
        // Handle edge cases
        if (daysLeft <= 0) {
            // Exam has passed or is today - very low priority
            priority = -1000.0 + daysLeft;  // More negative for older exams
            return;
        }
        
        if (completionRatio >= 1.0) {
            // Course is complete - very low priority
            priority = 0.0;
            return;
        }
        
        // Improved priority formula with better scaling
        // Base urgency factor (higher for closer exams)
        double urgencyFactor = 100.0 / (daysLeft + 1);
        
        // Incomplete work factor (higher for less complete work)
        double incompleteWork = 1.0 - completionRatio;
        
        // Difficulty multiplier
        double difficultyMultiplier = difficulty / 5.0;
        
        // Final priority calculation
        priority = urgencyFactor * incompleteWork * difficultyMultiplier * 10.0;
        
        // Ensure priority is positive for valid future exams
        if (priority < 0) priority = 0.1;
    }
    
    // Polymorphism - Override virtual functions
    void displayInfo() const override {
        cout << "Course: " << name << endl;
        cout << "Difficulty: " << difficulty << "/5" << endl;
        cout << "Exam Date: " << examDate << endl;
        cout << "Hours: " << hoursCompleted << "/" << totalHoursNeeded << endl;
        cout << "Priority: " << fixed << setprecision(2) << priority << endl;
        
        int daysLeft = getDaysUntilExam();
        if (daysLeft > 0) {
            cout << "Days until exam: " << daysLeft << endl;
        } else if (daysLeft == 0) {
            cout << "Exam is TODAY!" << endl;
        } else {
            cout << "Exam was " << abs(daysLeft) << " days ago" << endl;
        }
    }
    
    double getPriority() const override {
        return priority;
    }
    
    // Serialization for persistence
    string serialize() const override {
        stringstream ss;
        ss << name << "," << difficulty << "," << examDate << "," 
           << totalHoursNeeded << "," << hoursCompleted;
        return ss.str();
    }
    
    void deserialize(const string& data) override {
        stringstream ss(data);
        string token;
        
        getline(ss, name, ',');
        getline(ss, token, ','); difficulty = stoi(token);
        getline(ss, examDate, ',');
        getline(ss, token, ','); totalHoursNeeded = stoi(token);
        getline(ss, token, ','); hoursCompleted = stoi(token);
        
        calculatePriority();
    }
};

// ==================== TIME SLOT CLASS ====================
class TimeSlot {
private:
    string day;
    string startTime;
    string endTime;
    bool available;
    
public:
    TimeSlot(const string& d, const string& start, const string& end)
        : day(d), startTime(start), endTime(end), available(true) {}
    
    // Getters
    string getDay() const { return day; }
    string getStartTime() const { return startTime; }
    string getEndTime() const { return endTime; }
    bool isAvailable() const { return available; }
    
    void setAvailable(bool avail) { available = avail; }
    
    // Calculate duration in hours
    int getDurationHours() const {
        // Simplified - parse HH:MM format
        int startHour = stoi(startTime.substr(0, 2));
        int endHour = stoi(endTime.substr(0, 2));
        return endHour - startHour;
    }
    
    void displayInfo() const {
        cout << day << " " << startTime << "-" << endTime 
             << " (" << getDurationHours() << "h) " 
             << (available ? "Available" : "Booked") << endl;
    }
};

// ==================== STUDY SESSION CLASS ====================
class StudySession {
private:
    shared_ptr<Course> course;
    TimeSlot timeSlot;
    int duration;
    
public:
    StudySession(shared_ptr<Course> c, const TimeSlot& slot, int dur)
        : course(c), timeSlot(slot), duration(dur) {}
    
    shared_ptr<Course> getCourse() const { return course; }
    TimeSlot getTimeSlot() const { return timeSlot; }
    int getDuration() const { return duration; }
    
    void displayInfo() const {
        cout << "Study Session: " << course->getName() << endl;
        cout << "Time: " << timeSlot.getDay() << " " 
             << timeSlot.getStartTime() << "-" << timeSlot.getEndTime() << endl;
        cout << "Duration: " << duration << " hours" << endl;
    }
};

// ==================== SCHEDULE CLASS ====================
class Schedule : public Persistable {
private:
    vector<StudySession> sessions;
    string scheduleName;
    
public:
    Schedule(const string& name) : scheduleName(name) {}
    
    void addSession(const StudySession& session) {
        sessions.push_back(session);
    }
    
    void displaySchedule() const {
        cout << "\n=== " << scheduleName << " ===" << endl;
        cout << "Total Sessions: " << sessions.size() << endl;
        
        if (sessions.empty()) {
            cout << "No study sessions scheduled." << endl;
            cout << "This could mean:" << endl;
            cout << "- All courses are complete" << endl;
            cout << "- No available time slots" << endl;
            cout << "- All exam dates have passed" << endl;
            return;
        }
        
        cout << "\nDetailed Schedule:" << endl;
        cout << string(50, '-') << endl;
        
        for (const auto& session : sessions) {
            session.displayInfo();
            cout << string(30, '-') << endl;
        }
    }
    
    int getTotalStudyHours() const {
        int total = 0;
        for (const auto& session : sessions) {
            total += session.getDuration();
        }
        return total;
    }
    
    // Serialization
    string serialize() const override {
        stringstream ss;
        ss << scheduleName << "\n";
        ss << sessions.size() << "\n";
        for (const auto& session : sessions) {
            ss << session.getCourse()->getName() << ","
               << session.getTimeSlot().getDay() << ","
               << session.getTimeSlot().getStartTime() << ","
               << session.getTimeSlot().getEndTime() << ","
               << session.getDuration() << "\n";
        }
        return ss.str();
    }
    
    void deserialize(const string& data) override {
        // Implementation for loading saved schedules
        // This would parse the serialized data and reconstruct the schedule
    }
};

// ==================== SCHEDULE OPTIMIZER CLASS (MAIN LOGIC) ====================
class ScheduleOptimizer {
private:
    vector<shared_ptr<Course>> courses;
    vector<TimeSlot> availableSlots;
    
    // Strategy pattern for different optimization algorithms
    enum OptimizationStrategy {
        PRIORITY_BASED,
        TIME_BALANCED,
        DIFFICULTY_FIRST
    };
    
    OptimizationStrategy strategy;
    
public:
    ScheduleOptimizer() : strategy(PRIORITY_BASED) {}
    
    // Add course to the system
    void addCourse(shared_ptr<Course> course) {
        courses.push_back(course);
    }
    
    // Add available time slot
    void addTimeSlot(const TimeSlot& slot) {
        availableSlots.push_back(slot);
    }
    
    // Set optimization strategy
    void setStrategy(OptimizationStrategy strat) {
        strategy = strat;
    }
    
    // IMPROVED: Generate optimized schedule with better logic
    Schedule generateSchedule() {
        Schedule schedule("Optimized Study Schedule");
        
        // Filter out completed courses and past exams
        vector<shared_ptr<Course>> activeCourses;
        for (auto& course : courses) {
            if (course->getRemainingHours() > 0 && course->getPriority() > 0) {
                activeCourses.push_back(course);
            }
        }
        
        if (activeCourses.empty()) {
            cout << "No active courses to schedule!" << endl;
            return schedule;
        }
        
        // Sort courses by priority (polymorphism in action)
        sort(activeCourses.begin(), activeCourses.end(), 
             [](const shared_ptr<Course>& a, const shared_ptr<Course>& b) {
                 return a->getPriority() > b->getPriority();
             });
        
        // Create a copy of available slots to modify
        vector<TimeSlot> workingSlots = availableSlots;
        
        // Allocate study sessions
        for (auto& course : activeCourses) {
            int remainingHours = course->getRemainingHours();
            
            for (auto& slot : workingSlots) {
                if (remainingHours <= 0) break;
                if (!slot.isAvailable()) continue;
                
                int sessionHours = min(remainingHours, slot.getDurationHours());
                
                if (sessionHours > 0) {
                    StudySession session(course, slot, sessionHours);
                    schedule.addSession(session);
                    
                    slot.setAvailable(false);
                    remainingHours -= sessionHours;
                }
            }
        }
        
        return schedule;
    }
    
    // Display all courses
    void displayCourses() const {
        cout << "\n=== COURSES OVERVIEW ===" << endl;
        if (courses.empty()) {
            cout << "No courses added yet." << endl;
            return;
        }
        
        for (const auto& course : courses) {
            course->displayInfo();
            cout << string(40, '-') << endl;
        }
    }
    
    // Display available time slots
    void displayTimeSlots() const {
        cout << "\n=== AVAILABLE TIME SLOTS ===" << endl;
        if (availableSlots.empty()) {
            cout << "No time slots added yet." << endl;
            return;
        }
        
        for (const auto& slot : availableSlots) {
            slot.displayInfo();
        }
    }
    
    // Save schedule to file
    void saveSchedule(const Schedule& schedule, const string& filename) {
        ofstream file(filename);
        if (file.is_open()) {
            file << schedule.serialize();
            file.close();
            cout << "Schedule saved to " << filename << endl;
        } else {
            cout << "Error saving schedule!" << endl;
        }
    }
    
    // Get statistics
    void displayStatistics() const {
        cout << "\n=== STUDY STATISTICS ===" << endl;
        cout << "Total Courses: " << courses.size() << endl;
        cout << "Available Time Slots: " << availableSlots.size() << endl;
        
        if (courses.empty()) {
            cout << "No courses to analyze." << endl;
            return;
        }
        
        int totalHours = 0, completedHours = 0, activeCourses = 0;
        for (const auto& course : courses) {
            totalHours += course->getTotalHours();
            completedHours += course->getCompletedHours();
            if (course->getPriority() > 0) activeCourses++;
        }
        
        cout << "Active Courses: " << activeCourses << endl;
        cout << "Total Study Hours Needed: " << totalHours << endl;
        cout << "Hours Completed: " << completedHours << endl;
        cout << "Remaining Hours: " << (totalHours - completedHours) << endl;
        
        if (totalHours > 0) {
            double percentage = (double)completedHours / totalHours * 100;
            cout << "Completion Percentage: " << fixed << setprecision(1) 
                 << percentage << "%" << endl;
        }
    }
    
    // Add this test function to verify the date calculation
    void testDateCalculation() const {
        cout << "\n=== DATE CALCULATION TEST ===" << endl;
        
        // Test with August 15, 2025
        Course testCourse("Test Course", 3, "2025-08-15", 10);
        int days = testCourse.getDaysUntilExam();
        
        cout << "Days until August 15, 2025: " << days << endl;
        cout << "Expected: 28 days" << endl;
        
        // Test with August 17, 2025
        Course testCourse2("Test Course 2", 3, "2025-08-17", 10);
        int days2 = testCourse2.getDaysUntilExam();
        
        cout << "Days until August 17, 2025: " << days2 << endl;
        cout << "Expected: 30 days" << endl;
        
        // Show current date for verification
        time_t now = time(0);
        tm* timeinfo = localtime(&now);
        cout << "Current date: " << (timeinfo->tm_year + 1900) << "-" 
             << setfill('0') << setw(2) << (timeinfo->tm_mon + 1) << "-" 
             << setfill('0') << setw(2) << timeinfo->tm_mday << endl;
    }
};

// ==================== USER INTERFACE CLASS ====================
class UserInterface {
private:
    ScheduleOptimizer optimizer;
    
public:
    void displayMenu() {
        cout << "\n" << string(50, '=') << endl;
        cout << "      STUDY SCHEDULE OPTIMIZER" << endl;
        cout << string(50, '=') << endl;
        cout << "1. Add Course" << endl;
        cout << "2. Add Time Slot" << endl;
        cout << "3. View Courses" << endl;
        cout << "4. View Time Slots" << endl;
        cout << "5. Generate Schedule" << endl;
        cout << "6. View Statistics" << endl;
        cout << "7. Load Sample Data" << endl;
        cout << "8. Test Date Calculation" << endl;
        cout << "9. Exit" << endl;
        cout << string(50, '=') << endl;
        cout << "Enter your choice: ";
    }
    
    void addCourse() {
        string name, examDate;
        int difficulty, totalHours;
        
        cout << "\n--- Add New Course ---" << endl;
        cout << "Course Name: ";
        cin.ignore();
        getline(cin, name);
        
        do {
            cout << "Difficulty (1-5): ";
            cin >> difficulty;
            if (difficulty < 1 || difficulty > 5) {
                cout << "Please enter a difficulty between 1 and 5." << endl;
            }
        } while (difficulty < 1 || difficulty > 5);
        
        cout << "Exam Date (YYYY-MM-DD): ";
        cin >> examDate;
        
        do {
            cout << "Total Study Hours Needed: ";
            cin >> totalHours;
            if (totalHours <= 0) {
                cout << "Please enter a positive number of hours." << endl;
            }
        } while (totalHours <= 0);
        
        auto course = make_shared<Course>(name, difficulty, examDate, totalHours);
        optimizer.addCourse(course);
        
        cout << "Course added successfully!" << endl;
    }
    
    void addTimeSlot() {
         string daysInput, startTime, endTime;

        cout << "\n--- Add Time Slot ---" << endl;
        cin.ignore(); // Clear leftover newline from previous input
        cout << "Day(s) (e.g., Monday to monday, Tuesday ): ";
        getline(cin, daysInput);

        cout << "Start Time (HH:MM, e.g., 09:00): ";
        cin >> startTime;

        cout << "End Time (HH:MM, e.g., 12:00): ";
        cin >> endTime;

        // Split and trim days
        stringstream ss(daysInput);
        string day;
        while (getline(ss, day, ',')) {
            // Trim leading and trailing spaces
            day.erase(0, day.find_first_not_of(" \t"));
            day.erase(day.find_last_not_of(" \t") + 1);

            if (!day.empty()) {
                TimeSlot slot(day, startTime, endTime);
                optimizer.addTimeSlot(slot);
                cout << "Time slot for " << day << " added successfully!" << endl;
            }
        }
    }
    
    void loadSampleData() {
        cout << "\nLoading sample data..." << endl;
        
        // Get current date for creating realistic future exam dates
        time_t now = time(0);
        tm* timeinfo = localtime(&now);
        int currentYear = timeinfo->tm_year + 1900;
        int currentMonth = timeinfo->tm_mon + 1;
        
        // Create exam dates 1-4 months in the future
        string examDates[4];
        int futureMonths[] = {1, 2, 3, 4};
        
        for (int i = 0; i < 4; i++) {
            int examMonth = currentMonth + futureMonths[i];
            int examYear = currentYear;
            
            if (examMonth > 12) {
                examMonth -= 12;
                examYear++;
            }
            
            stringstream ss;
            ss << examYear << "-" << setfill('0') << setw(2) << examMonth << "-15";
            examDates[i] = ss.str();
        }
        
        // Sample courses with realistic future dates
        optimizer.addCourse(make_shared<Course>("Data Structures", 4, examDates[0], 25));
        optimizer.addCourse(make_shared<Course>("Algorithm Analysis", 5, examDates[1], 30));
        optimizer.addCourse(make_shared<Course>("Database Systems", 3, examDates[2], 20));
        optimizer.addCourse(make_shared<Course>("Software Engineering", 3, examDates[3], 22));
        
        // Sample time slots
        optimizer.addTimeSlot(TimeSlot("Monday", "09:00", "12:00"));
        optimizer.addTimeSlot(TimeSlot("Monday", "14:00", "17:00"));
        optimizer.addTimeSlot(TimeSlot("Tuesday", "09:00", "12:00"));
        optimizer.addTimeSlot(TimeSlot("Tuesday", "14:00", "17:00"));
        optimizer.addTimeSlot(TimeSlot("Wednesday", "09:00", "12:00"));
        optimizer.addTimeSlot(TimeSlot("Wednesday", "14:00", "17:00"));
        optimizer.addTimeSlot(TimeSlot("Thursday", "09:00", "12:00"));
        optimizer.addTimeSlot(TimeSlot("Thursday", "14:00", "17:00"));
        optimizer.addTimeSlot(TimeSlot("Friday", "09:00", "12:00"));
        optimizer.addTimeSlot(TimeSlot("Friday", "14:00", "17:00"));
        optimizer.addTimeSlot(TimeSlot("Saturday", "10:00", "13:00"));
        optimizer.addTimeSlot(TimeSlot("Sunday", "10:00", "13:00"));
        
        cout << "Sample data loaded successfully!" << endl;
        cout << "Courses created with exam dates 1-4 months in the future." << endl;
    }
    
    void run() {
        int choice;
        
        cout << "Welcome to Study Schedule Optimizer!" << endl;
        cout << "This system helps you create optimal study schedules." << endl;
        cout << "Start by loading sample data or adding your own courses and time slots." << endl;
        
        while (true) {
            displayMenu();
            cin >> choice;
            
            switch (choice) {
                case 1:
                    addCourse();
                    break;
                case 2:
                    addTimeSlot();
                    break;
                case 3:
                    optimizer.displayCourses();
                    break;
                case 4:
                    optimizer.displayTimeSlots();
                    break;
                case 5: {
                    Schedule schedule = optimizer.generateSchedule();
                    schedule.displaySchedule();
                    
                    cout << "\nSave schedule to file? (y/n): ";
                    char save;
                    cin >> save;
                    if (save == 'y' || save == 'Y') {
                        optimizer.saveSchedule(schedule, "study_schedule.txt");
                    }
                    break;
                }
                case 6:
                    optimizer.displayStatistics();
                    break;
                case 7:
                    loadSampleData();
                    break;
                case 8:
                    optimizer.testDateCalculation();
                    break;
                case 9:
                    cout << "Thank you for using Study Schedule Optimizer!" << endl;
                    return;
                default:
                    cout << "Invalid choice. Please try again." << endl;
            }
            
            cout << "\nPress Enter to continue...";
            cin.ignore();
            cin.get();
        }
    }
};

// ==================== MAIN FUNCTION ====================
int main() {
    try {
        UserInterface ui;
        ui.run();
    } catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}