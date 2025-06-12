# EEESEABOAT 2025 GROUP 21

# Frontend:
A React-based interface for Arduino EEESEABOAT control system.

## Prerequisites

- **Node.js** (version 18 or higher)
- **npm** (comes with Node.js)

## Installation & Setup

1. **Clone the repository:**
   ```bash
   git clone https://github.com/vortexisalpha/eeeseaboat2025.git
   cd eeeseaboat2025
   ```

2. **Navigate to the frontend directory:**
   ```bash
   cd frontend
   ```

3. **Install dependencies:**
   ```bash
   npm install
   ```

4. **Start the development server:**
   ```bash
   npm run dev
   ```

   The application will be available at `http://localhost:5173` (or another port if 5173 is in use).

# Backend: 

## How to run:

1. Navigate to **backend/final_single_backend.ino** and download the arduino IDE.

2. Open this with the **arduino IDE** and put connect arduino with **wifi module** attached.

3. Push the script to the arduino and ensure the **serial monitor** is open when the script is uploaded (cmd + shift + m)

4. Ensure SSID and PASS are the same as the wifi your computer is connected to then enter the ip address in **frontend/src/services/arduinoService.ts** at the top

5. **npm run dev** instruction as stated above.