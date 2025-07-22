#include "httplib.h"
#include "json.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using json = nlohmann::json;
using namespace httplib;

// 🔒 Encapsulation: Course class with private members and public accessors
class Course {
private:
    std::string name;
    int difficulty;
    std::string examDate;

public:
    Course(const std::string& name, int difficulty, const std::string& examDate)
        : name(name), difficulty(difficulty), examDate(examDate) {}

    std::string getName() const { return name; }
    int getDifficulty() const { return difficulty; }
    std::string getExamDate() const { return examDate; }
};

// 🎯 Abstraction: Base User class
class User {
protected:
    std::string name;

public:
    User(const std::string& name) : name(name) {}
    virtual std::string displayPlan() const = 0; // Pure virtual function
    virtual ~User() = default;
};

// 🧠 Inheritance + 🔁 Polymorphism: Student inherits from User and overrides displayPlan
class Student : public User {
private:
    std::vector<Course> courses;

public:
    Student(const std::string& name) : User(name) {}

    void addCourse(const Course& course) {
        courses.push_back(course);
    }

    std::string displayPlan() const override {
        std::ostringstream plan;
        plan << "<p>Hello <strong>" << name << "</strong>, here is your study plan:</p>";
        plan << "<table border='1'><tr><th>Course</th><th>Difficulty</th><th>Exam Date</th><th>Daily Hours</th></tr>";

        for (const auto& course : courses) {
            int hours = course.getDifficulty() * 2;
            plan << "<tr><td>" << course.getName() << "</td><td>" << course.getDifficulty()
                 << "</td><td>" << course.getExamDate() << "</td><td>" << hours << " hrs/day</td></tr>";
        }

        plan << "</table>";
        return plan.str();
    }
};

// 📄 Read file content (for serving static files)
std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) return "";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main() {
    Server svr;

    // Serve HTML
    svr.Get("/", [](const Request&, Response& res) {
        std::string content = read_file("frontend/index.html");
        res.set_content(content, "text/html");
    });

    // Serve CSS
    svr.Get("/style.css", [](const Request&, Response& res) {
        std::string content = read_file("frontend/style.css");
        res.set_content(content, "text/css");
    });

    // Serve JS
    svr.Get("/script.js", [](const Request&, Response& res) {
        std::string content = read_file("frontend/script.js");
        res.set_content(content, "application/javascript");
    });

    // Handle POST request to generate plan
    svr.Post("/generate-plan", [](const Request& req, Response& res) {
        try {
            auto data = json::parse(req.body);
            std::string student_name = data["name"];
            Student student(student_name); // 🧑‍🎓 Using polymorphic class

            for (const auto& course : data["courses"]) {
                student.addCourse(Course(
                    course["name"],
                    course["difficulty"],
                    course["exam_date"]
                ));
            }

            std::string result_html = student.displayPlan();
            res.set_content(result_html, "text/html");
        } catch (const std::exception& e) {
            res.status = 400;
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    std::cout << "✅ Server running at http://localhost:8080" << std::endl;
    svr.listen("localhost", 8080);
}
