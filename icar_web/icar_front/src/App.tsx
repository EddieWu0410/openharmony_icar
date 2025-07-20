import { useState } from "react";
import "./index.css";
import { StatusPanel, ControllerPanel } from "./components";

function App() {
  // 当前显示的面板：'controller' | 'status'
  const [currentPanel, setCurrentPanel] = useState<"controller" | "status">(
    "controller"
  );

  // 显示状态面板
  const showStatusPanel = () => {
    setCurrentPanel("status");
  };

  // 显示控制面板
  const showControllerPanel = () => {
    setCurrentPanel("controller");
  };

  return (
    <div className="min-h-screen bg-dark-bg">
      <div className="container mx-auto p-5 max-w-7xl">
        {/* 根据当前面板状态渲染对应组件 */}
        {currentPanel === "status" ? (
          <StatusPanel onClose={showControllerPanel} />
        ) : (
          <ControllerPanel onShowStatus={showStatusPanel} />
        )}

        {/* 公共页脚 */}
        <footer className="bg-dark-bg text-center py-5 border-t-2 border-cyan-400/30 mt-10">
          <p className="text-light-text opacity-80">
            机器小车控制系统 &copy; 2025
          </p>
        </footer>
      </div>
    </div>
  );
}

export default App;
