# EEESEABOAT 2025

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

## Available Scripts

In the `frontend` directory, you can run:

- **`npm run dev`** - Starts the development server
- **`npm run build`** - Builds the app for production
- **`npm run preview`** - Preview the production build locally
- **`npm run lint`** - Run ESLint to check for code issues

## Project Structure

```
eeeseaboat2025/
├── frontend/          # React application
│   ├── src/          # Source code
│   ├── public/       # Public assets
│   ├── package.json  # Frontend dependencies
│   └── ...
└── README.md         # This file
```

## Technology Stack

- **React 19** - Frontend framework
- **TypeScript** - Type safety
- **Vite** - Build tool and dev server
- **Chakra UI** - Component library
- **Axios** - HTTP client
- **Framer Motion** - Animations

## Troubleshooting

If you encounter issues:

1. **Delete node_modules and reinstall:**
   ```bash
   cd frontend
   rm -rf node_modules package-lock.json
   npm install
   ```

2. **Check Node.js version:**
   ```bash
   node --version
   ```
   Make sure you're using Node.js 18 or higher.

3. **Clear npm cache:**
   ```bash
   npm cache clean --force
   ```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test your changes
5. Submit a pull request

## License

This project is part of the EEESEABOAT 2025 competition. 