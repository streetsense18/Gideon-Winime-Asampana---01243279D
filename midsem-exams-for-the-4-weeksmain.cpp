
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <limits>

using namespace std;

// ---------- Helpers ----------
static inline string trim(const string &s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

static inline void pause() {
    cout << "\nPress ENTER to continue...";
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

static inline int readInt(const string& prompt, int minV, int maxV) {
    while (true) {
        cout << prompt;
        int x;
        if (cin >> x) {
            if (x >= minV && x <= maxV) {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                return x;
            }
        }
        cout << "Invalid input. Try again.\n";
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

static inline string readLineNonEmpty(const string& prompt) {
    while (true) {
        cout << prompt;
        string s;
        getline(cin, s);
        s = trim(s);
        if (!s.empty()) return s;
        cout << "Input cannot be empty. Try again.\n";
    }
}

static inline bool fileExists(const string& path) {
    ifstream f(path);
    return f.good();
}

// ---------- Models ----------
struct Student {
    string indexNo;
    string fullName;
    string program;

    // line format: index|name|program
    string serialize() const {
        return indexNo + "|" + fullName + "|" + program;
    }

    static bool deserialize(const string& line, Student& out) {
        stringstream ss(line);
        string a,b,c;
        if (!getline(ss, a, '|')) return false;
        if (!getline(ss, b, '|')) return false;
        if (!getline(ss, c)) return false;
        out.indexNo = trim(a);
        out.fullName = trim(b);
        out.program = trim(c);
        return !(out.indexNo.empty() || out.fullName.empty());
    }
};

enum class Status { Present, Absent, Late };

static inline string statusToString(Status s) {
    switch (s) {
        case Status::Present: return "Present";
        case Status::Absent:  return "Absent";
        case Status::Late:    return "Late";
    }
    return "Absent";
}

static inline bool stringToStatus(const string& s, Status& out) {
    string t = s;
    for (auto& ch : t) ch = (char)tolower(ch);
    t = trim(t);
    if (t == "present") { out = Status::Present; return true; }
    if (t == "absent")  { out = Status::Absent;  return true; }
    if (t == "late")    { out = Status::Late;    return true; }
    return false;
}

struct AttendanceRecord {
    string indexNo;
    Status status;
};

struct AttendanceSession {
    string courseCode;   // e.g. EE201
    string date;         // YYYY-MM-DD
    string startTime;    // HH:MM
    int durationMins;    // minutes
    vector<AttendanceRecord> records; // one per registered student

    string filename() const {
        // session_EE201_YYYY_MM_DD.txt
        string dateUnderscore = date;
        for (auto& ch : dateUnderscore) if (ch == '-') ch = '_';
        return "session_" + courseCode + "_" + dateUnderscore + ".txt";
    }

    // File format:
    // COURSE|DATE|START|DURATION
    // index|status
    bool saveToFile() const {
        ofstream out(filename());
        if (!out) return false;
        out << courseCode << "|" << date << "|" << startTime << "|" << durationMins << "\n";
        for (const auto& r : records) {
            out << r.indexNo << "|" << statusToString(r.status) << "\n";
        }
        return true;
    }

    static bool loadFromFile(const string& file, AttendanceSession& sess) {
        ifstream in(file);
        if (!in) return false;

        string header;
        if (!getline(in, header)) return false;

        stringstream ss(header);
        string a,b,c,d;
        if (!getline(ss, a, '|')) return false;
        if (!getline(ss, b, '|')) return false;
        if (!getline(ss, c, '|')) return false;
        if (!getline(ss, d)) return false;

        sess.courseCode = trim(a);
        sess.date = trim(b);
        sess.startTime = trim(c);
        sess.durationMins = stoi(trim(d));

        sess.records.clear();
        string line;
        while (getline(in, line)) {
            line = trim(line);
            if (line.empty()) continue;
            stringstream rs(line);
            string idx, st;
            if (!getline(rs, idx, '|')) continue;
            if (!getline(rs, st)) continue;
            AttendanceRecord rec;
            rec.indexNo = trim(idx);
            Status s;
            if (!stringToStatus(st, s)) s = Status::Absent;
            rec.status = s;
            sess.records.push_back(rec);
        }
        return true;
    }
};

// ---------- Storage ----------
const string STUDENTS_FILE = "students.txt";
const string SESSIONS_INDEX_FILE = "sessions_index.txt";

static inline void loadStudents(vector<Student>& students) {
    students.clear();
    ifstream in(STUDENTS_FILE);
    if (!in) return;

    string line;
    while (getline(in, line)) {
        Student s;
        if (Student::deserialize(line, s)) students.push_back(s);
    }
}

static inline bool saveStudents(const vector<Student>& students) {
    ofstream out(STUDENTS_FILE);
    if (!out) return false;
    for (const auto& s : students) out << s.serialize() << "\n";
    return true;
}

static inline vector<string> loadSessionFilenames() {
    vector<string> files;
    ifstream in(SESSIONS_INDEX_FILE);
    if (!in) return files;
    string line;
    while (getline(in, line)) {
        line = trim(line);
        if (!line.empty()) files.push_back(line);
    }
    // remove duplicates
    sort(files.begin(), files.end());
    files.erase(unique(files.begin(), files.end()), files.end());
    return files;
}

static inline bool saveSessionFilenames(const vector<string>& files) {
    ofstream out(SESSIONS_INDEX_FILE);
    if (!out) return false;
    for (const auto& f : files) out << f << "\n";
    return true;
}

static inline bool indexNoExists(const vector<Student>& students, const string& idx) {
    return any_of(students.begin(), students.end(), [&](const Student& s){ return s.indexNo == idx; });
}

static inline const Student* findStudent(const vector<Student>& students, const string& idx) {
    for (const auto& s : students) if (s.indexNo == idx) return &s;
    return nullptr;
}

// ---------- Features ----------
static void registerStudent(vector<Student>& students) {
    cout << "\n--- Register Student ---\n";
    string idx = readLineNonEmpty("Index Number: ");
    if (indexNoExists(students, idx)) {
        cout << "Student with this index number already exists.\n";
        return;
    }
    string name = readLineNonEmpty("Full Name: ");
    string program = readLineNonEmpty("Programme (e.g., HND Electrical Engineering L200): ");

    students.push_back({idx, name, program});
    if (saveStudents(students)) cout << "Saved.\n";
    else cout << "ERROR: Could not save students to file.\n";
}

static void viewStudents(const vector<Student>& students) {
    cout << "\n--- All Registered Students ---\n";
    if (students.empty()) {
        cout << "No students registered.\n";
        return;
    }
    cout << left << setw(18) << "Index No" << setw(30) << "Full Name" << "Programme\n";
    cout << string(80, '-') << "\n";
    for (const auto& s : students) {
        cout << left << setw(18) << s.indexNo << setw(30) << s.fullName << s.program << "\n";
    }
}

static void searchStudent(const vector<Student>& students) {
    cout << "\n--- Search Student ---\n";
    string idx = readLineNonEmpty("Enter Index Number: ");
    const Student* s = findStudent(students, idx);
    if (!s) {
        cout << "Not found.\n";
        return;
    }
    cout << "Found:\n";
    cout << "Index: " << s->indexNo << "\nName : " << s->fullName << "\nProg : " << s->program << "\n";
}

static AttendanceSession createSession(const vector<Student>& students) {
    cout << "\n--- Create Lecture Session ---\n";
    AttendanceSession sess;
    sess.courseCode = readLineNonEmpty("Course Code (e.g., EE201): ");
    sess.date = readLineNonEmpty("Date (YYYY-MM-DD): ");
    sess.startTime = readLineNonEmpty("Start Time (HH:MM): ");
    sess.durationMins = readInt("Duration (minutes): ", 1, 10000);

    // default all to Absent (or choose Present; requirement allows any, but Absent is safer)
    sess.records.clear();
    for (const auto& st : students) {
        sess.records.push_back({st.indexNo, Status::Absent});
    }

    cout << "Session created: " << sess.courseCode << " on " << sess.date
         << " at " << sess.startTime << " (" << sess.durationMins << " mins)\n";
    return sess;
}

static void markAttendance(AttendanceSession& sess, const vector<Student>& students) {
    cout << "\n--- Mark Attendance ---\n";
    if (students.empty()) {
        cout << "No students registered.\n";
        return;
    }

    cout << "Choose marking mode:\n";
    cout << "1) Mark one student\n";
    cout << "2) Mark all students (step-through)\n";
    int mode = readInt("Select (1-2): ", 1, 2);

    auto setStatusForIndex = [&](const string& idx, Status s) {
        for (auto& r : sess.records) {
            if (r.indexNo == idx) { r.status = s; return true; }
        }
        return false;
    };

    if (mode == 1) {
        string idx = readLineNonEmpty("Enter student index number: ");
        if (!indexNoExists(students, idx)) {
            cout << "This index number is not registered.\n";
            return;
        }
        cout << "Status options: 1=Present, 2=Absent, 3=Late\n";
        int c = readInt("Choose status (1-3): ", 1, 3);
        Status s = (c==1?Status::Present:(c==2?Status::Absent:Status::Late));
        setStatusForIndex(idx, s);
        cout << "Updated.\n";
    } else {
        cout << "Status options: 1=Present, 2=Absent, 3=Late\n";
        for (const auto& st : students) {
            cout << "\nStudent: " << st.indexNo << " - " << st.fullName << "\n";
            int c = readInt("Choose status (1-3): ", 1, 3);
            Status s = (c==1?Status::Present:(c==2?Status::Absent:Status::Late));
            setStatusForIndex(st.indexNo, s);
        }
        cout << "All attendance captured.\n";
    }
}

static void updateAttendance(AttendanceSession& sess, const vector<Student>& students) {
    cout << "\n--- Update Attendance Record ---\n";
    string idx = readLineNonEmpty("Enter student index number: ");
    if (!indexNoExists(students, idx)) {
        cout << "This index number is not registered.\n";
        return;
    }
    cout << "Status options: 1=Present, 2=Absent, 3=Late\n";
    int c = readInt("Choose new status (1-3): ", 1, 3);
    Status s = (c==1?Status::Present:(c==2?Status::Absent:Status::Late));

    bool ok = false;
    for (auto& r : sess.records) {
        if (r.indexNo == idx) { r.status = s; ok = true; break; }
    }
    if (ok) cout << "Record updated.\n";
    else cout << "ERROR: Record not found in this session.\n";
}

static void showSessionReport(const AttendanceSession& sess, const vector<Student>& students) {
    cout << "\n--- Attendance List (Session Report) ---\n";
    cout << "Course: " << sess.courseCode << " | Date: " << sess.date
         << " | Start: " << sess.startTime << " | Duration: " << sess.durationMins << " mins\n\n";

    cout << left << setw(18) << "Index No" << setw(30) << "Name" << "Status\n";
    cout << string(70, '-') << "\n";

    for (const auto& r : sess.records) {
        const Student* st = findStudent(students, r.indexNo);
        string nm = st ? st->fullName : "(Unknown)";
        cout << left << setw(18) << r.indexNo << setw(30) << nm << statusToString(r.status) << "\n";
    }
}

static void showSessionSummary(const AttendanceSession& sess) {
    int present=0, absent=0, late=0;
    for (const auto& r : sess.records) {
        if (r.status == Status::Present) present++;
        else if (r.status == Status::Late) late++;
        else absent++;
    }
    cout << "\n--- Summary Counts ---\n";
    cout << "Present: " << present << "\n";
    cout << "Late   : " << late << "\n";
    cout << "Absent : " << absent << "\n";
    cout << "Total  : " << (present+late+absent) << "\n";
}

static int pickSavedSession(const vector<string>& files) {
    cout << "\n--- Saved Sessions ---\n";
    if (files.empty()) {
        cout << "No saved sessions.\n";
        return -1;
    }
    for (size_t i=0; i<files.size(); i++) {
        cout << (i+1) << ") " << files[i] << "\n";
    }
    int choice = readInt("Choose session (1 - " + to_string((int)files.size()) + "): ", 1, (int)files.size());
    return choice - 1;
}

// ---------- Menus ----------
static void studentMenu(vector<Student>& students) {
    while (true) {
        cout << "\n====== Student Management ======\n";
        cout << "1) Register student\n";
        cout << "2) View all students\n";
        cout << "3) Search by index number\n";
        cout << "0) Back\n";
        int c = readInt("Select: ", 0, 3);
        if (c == 0) return;
        if (c == 1) registerStudent(students);
        else if (c == 2) viewStudents(students);
        else if (c == 3) searchStudent(students);
        pause();
    }
}

static void sessionMenu(vector<Student>& students) {
    vector<string> sessionFiles = loadSessionFilenames();
    AttendanceSession current;
    bool hasCurrent = false;

    while (true) {
        cout << "\n====== Attendance Session Management ======\n";
        cout << "1) Create new lecture session\n";
        cout << "2) Load saved session\n";
        cout << "3) Mark attendance (current session)\n";
        cout << "4) Update attendance record (current session)\n";
        cout << "5) Display attendance list (current session)\n";
        cout << "6) Display summary counts (current session)\n";
        cout << "7) Save current session\n";
        cout << "0) Back\n";

        int c = readInt("Select: ", 0, 7);

        if (c == 0) return;

        if (c == 1) {
            loadStudents(students); // refresh
            if (students.empty()) {
                cout << "Register students first before creating a session.\n";
            } else {
                current = createSession(students);
                hasCurrent = true;
            }
        }
        else if (c == 2) {
            int idx = pickSavedSession(sessionFiles);
            if (idx >= 0) {
                AttendanceSession loaded;
                if (AttendanceSession::loadFromFile(sessionFiles[idx], loaded)) {
                    current = loaded;
                    hasCurrent = true;
                    cout << "Loaded: " << sessionFiles[idx] << "\n";
                } else {
                    cout << "ERROR: Could not load session file.\n";
                }
            }
        }
        else if (c == 3) {
            loadStudents(students); // refresh
            if (!hasCurrent) cout << "No current session. Create or load one.\n";
            else markAttendance(current, students);
        }
        else if (c == 4) {
            loadStudents(students); // refresh
            if (!hasCurrent) cout << "No current session. Create or load one.\n";
            else updateAttendance(current, students);
        }
        else if (c == 5) {
            loadStudents(students); // refresh
            if (!hasCurrent) cout << "No current session. Create or load one.\n";
            else showSessionReport(current, students);
        }
        else if (c == 6) {
            if (!hasCurrent) cout << "No current session. Create or load one.\n";
            else showSessionSummary(current);
        }
        else if (c == 7) {
            if (!hasCurrent) {
                cout << "No current session. Create or load one.\n";
            } else {
                bool ok = current.saveToFile();
                if (!ok) {
                    cout << "ERROR: Could not save session file.\n";
                } else {
                    cout << "Saved session to: " << current.filename() << "\n";
                    sessionFiles.push_back(current.filename());
                    sort(sessionFiles.begin(), sessionFiles.end());
                    sessionFiles.erase(unique(sessionFiles.begin(), sessionFiles.end()), sessionFiles.end());
                    if (!saveSessionFilenames(sessionFiles)) {
                        cout << "WARNING: Could not update sessions_index.txt\n";
                    }
                }
            }
        }

        pause();
    }
}

int main() {
    vector<Student> students;
    loadStudents(students);

    while (true) {
        cout << "\n========================================\n";
        cout << " DIGITAL ATTENDANCE SYSTEM (C++)\n";
        cout << "========================================\n";
        cout << "1) Student Management\n";
        cout << "2) Attendance Session Management\n";
        cout << "0) Exit\n";

        int c = readInt("Select: ", 0, 2);
        if (c == 0) {
            cout << "Goodbye.\n";
            return 0;
        }
        if (c == 1) {
            loadStudents(students);
            studentMenu(students);
        } else if (c == 2) {
            loadStudents(students);
            sessionMenu(students);
        }
    }
}