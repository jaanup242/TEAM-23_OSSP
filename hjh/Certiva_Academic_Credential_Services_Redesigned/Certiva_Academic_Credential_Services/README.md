# Certiva — Academic Credential Services Platform

Flask + HTML/CSS/JS academic certificate request portal.

## Run
```bash
python backend/app.py
```
Open `http://127.0.0.1:5000`.

## Demo accounts
- Student: `2520030441` / `student123`
- Admin: `admin` / `admin123`

## Main flow
Home → Student Login → Student Dashboard → Apply for Certificate → Request ID → Track Status

Admin flow: Admin Login → Request Queue → Process → Complete

The admin dashboard also visualizes the Producer–Consumer bounded buffer and semaphore state.
