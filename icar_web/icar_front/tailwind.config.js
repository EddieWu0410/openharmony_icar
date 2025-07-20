/** @type {import('tailwindcss').Config} */
export default {
  content: [
    "./index.html",
    "./src/**/*.{js,ts,jsx,tsx}",
  ],
  theme: {
    extend: {
      colors: {
        'dark-bg': '#1a1a2e',
        'dark-accent': '#16213e',
        'light-text': '#e2e8f0',
        'dark-text': '#1a1a2e',
        'accent-color': '#00d9ff',
        'accent-rgb': '0, 217, 255',
        'border-color': 'rgba(0, 217, 255, 0.3)',
        'success-color': '#10b981',
        'danger-color': '#ef4444',
        'warning-color': '#f59e0b',
      },
      fontFamily: {
        'orbitron': ['Orbitron', 'monospace'],
      },
      backgroundImage: {
        'gradient-primary': 'linear-gradient(135deg, #0f172a 0%, #1e293b 50%, #334155 100%)',
        'gradient-button': 'linear-gradient(145deg, #16213e, #1a1a2e)',
      },
      animation: {
        'background-pulse': 'backgroundPulse 4s ease-in-out infinite alternate',
        'header-shine': 'headerShine 3s linear infinite',
        'section-glow': 'sectionGlow 2s ease-in-out infinite alternate',
        'pulse-custom': 'pulse 0.5s ease-in-out',
      },
    },
  },
  plugins: [],
}

