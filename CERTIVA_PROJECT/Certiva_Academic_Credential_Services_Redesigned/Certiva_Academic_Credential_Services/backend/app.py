from flask import Flask, jsonify, request, send_from_directory, session
from flask_cors import CORS
from datetime import datetime

# =========================================================
# CERTIVA — ACADEMIC CREDENTIAL SERVICES PLATFORM
# =========================================================

app = Flask(
    __name__,
    static_folder="../frontend",
    static_url_path=""
)

app.secret_key = "certiva-demo-secret"

CORS(
    app,
    supports_credentials=True
)


# =========================================================
# DEMO LOGIN DETAILS
# =========================================================

STUDENT = {
    "roll_no": "2520030441",
    "password": "student123",
    "name": "Jaahnavi"
}

ADMIN = {
    "username": "admin",
    "password": "admin123"
}


# =========================================================
# CERTIFICATE REQUEST DATABASE
# =========================================================

requests_db = [
    {
        "request_id": "CERT-2026-0001",
        "student_name": "Jaahnavi",
        "roll_no": "2520030441",
        "certificate_type": "Bonafide Certificate",
        "purpose": "Internship documentation",
        "urgency": "Normal",
        "status": "Processing",
        "created_at": "10 Sep 2026, 10:32 AM"
    },
    {
        "request_id": "CERT-2026-0002",
        "student_name": "Rahul",
        "roll_no": "2520030112",
        "certificate_type": "Study Certificate",
        "purpose": "Higher studies",
        "urgency": "High",
        "status": "Completed",
        "created_at": "08 Sep 2026, 09:10 AM"
    },
    {
        "request_id": "CERT-2026-0003",
        "student_name": "Priya",
        "roll_no": "2520030240",
        "certificate_type": "Character Certificate",
        "purpose": "Personal documentation",
        "urgency": "Normal",
        "status": "Pending",
        "created_at": "11 Sep 2026, 02:15 PM"
    }
]

next_id = 4


# =========================================================
# FRONTEND PAGE HELPER
# =========================================================

def page(path):
    return send_from_directory("../frontend", path)


# =========================================================
# PUBLIC PAGES
# =========================================================

@app.get("/")
def home():
    return page("index.html")


@app.get("/student-login")
def student_login_page():
    return page("auth/student-login.html")


@app.get("/admin-login")
def admin_login_page():
    return page("auth/admin-login.html")


# =========================================================
# STUDENT PAGES
# =========================================================

@app.get("/student")
def student_page():
    return page("student/student.html")


@app.get("/student/apply")
def student_apply_page():
    return page("student/apply.html")


# =========================================================
# ADMIN PAGE
# =========================================================

@app.get("/admin")
def admin_page():
    return page("admin/admin.html")


# =========================================================
# FRONTEND FILES
#
# IMPORTANT:
# The function is called serve_page, NOT static.
# Flask already has a built-in endpoint called "static".
# =========================================================

@app.get("/<path:p>")
def serve_page(p):
    return send_from_directory("../frontend", p)


# =========================================================
# STUDENT LOGIN API
# =========================================================

@app.post("/api/login/student")
def student_login():

    data = request.get_json() or {}

    roll_no = data.get("roll_no")
    password = data.get("password")

    if (
        roll_no == STUDENT["roll_no"]
        and password == STUDENT["password"]
    ):

        session["role"] = "student"
        session["roll_no"] = STUDENT["roll_no"]

        return jsonify(
            ok=True,
            name=STUDENT["name"]
        )

    return jsonify(
        ok=False,
        error="Invalid roll number or password."
    ), 401


# =========================================================
# ADMIN LOGIN API
# =========================================================

@app.post("/api/login/admin")
def admin_login():

    data = request.get_json() or {}

    username = data.get("username")
    password = data.get("password")

    if (
        username == ADMIN["username"]
        and password == ADMIN["password"]
    ):

        session["role"] = "admin"

        return jsonify(ok=True)

    return jsonify(
        ok=False,
        error="Invalid admin username or password."
    ), 401


# =========================================================
# SESSION API
# =========================================================

@app.get("/api/session")
def get_session():

    return jsonify(
        logged_in="role" in session,
        role=session.get("role"),
        roll_no=session.get("roll_no")
    )


# =========================================================
# LOGOUT
# =========================================================

@app.post("/api/logout")
def logout():

    session.clear()

    return jsonify(ok=True)


# =========================================================
# GET CERTIFICATE REQUESTS
# =========================================================

@app.get("/api/requests")
def get_requests():

    # -------------------------
    # STUDENT
    # -------------------------

    if session.get("role") == "student":

        student_requests = [
            item
            for item in requests_db
            if item["roll_no"] == session.get("roll_no")
        ]

        return jsonify(student_requests)

    # -------------------------
    # ADMIN
    # -------------------------

    if session.get("role") == "admin":

        return jsonify(requests_db)

    return jsonify(
        error="Login required"
    ), 401


# =========================================================
# CREATE CERTIFICATE REQUEST
# =========================================================

@app.post("/api/requests")
def create_request():

    global next_id

    # Only students can create requests
    if session.get("role") != "student":

        return jsonify(
            error="Student login required"
        ), 403

    data = request.get_json() or {}

    required_fields = [
        "certificate_type",
        "purpose",
        "urgency"
    ]

    # Validate required fields
    for field in required_fields:

        if not str(data.get(field, "")).strip():

            return jsonify(
                error="Please complete all required fields."
            ), 400

    # Generate request ID
    request_id = (
        f"CERT-{datetime.now().year}-{next_id:04d}"
    )

    next_id += 1

    # Create request
    new_request = {
        "request_id": request_id,
        "student_name": STUDENT["name"],
        "roll_no": STUDENT["roll_no"],
        "certificate_type": data["certificate_type"],
        "purpose": data["purpose"].strip(),
        "urgency": data["urgency"],
        "status": "Pending",
        "created_at": datetime.now().strftime(
            "%d %b %Y, %I:%M %p"
        )
    }

    requests_db.append(new_request)

    return jsonify(new_request), 201


# =========================================================
# UPDATE REQUEST STATUS
# =========================================================

@app.patch("/api/requests/<rid>")
def update_request(rid):

    # Only admin can update
    if session.get("role") != "admin":

        return jsonify(
            error="Admin login required"
        ), 403

    request_item = next(
        (
            item
            for item in requests_db
            if item["request_id"] == rid
        ),
        None
    )

    if not request_item:

        return jsonify(
            error="Request not found"
        ), 404

    data = request.get_json() or {}

    valid_statuses = [
        "Pending",
        "Processing",
        "Completed",
        "Rejected"
    ]

    new_status = data.get("status")

    if new_status not in valid_statuses:

        return jsonify(
            error="Invalid status"
        ), 400

    request_item["status"] = new_status

    return jsonify(request_item)


# =========================================================
# RUN CERTIVA
# =========================================================

if __name__ == "__main__":

    print()
    print("======================================")
    print("       CERTIVA IS RUNNING")
    print("======================================")
    print()
    print("Student Login")
    print("Roll No : 2520030441")
    print("Password: student123")
    print()
    print("Admin Login")
    print("Username: admin")
    print("Password: admin123")
    print()
    print("Student Apply Page:")
    print("http://127.0.0.1:5000/student/apply")
    print()
    print("Home:")
    print("http://127.0.0.1:5000")
    print()
    print("======================================")

    app.run(
        host="127.0.0.1",
        port=5000,
        debug=True
    )