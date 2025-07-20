import { useState, useEffect, useCallback } from 'react';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import {
  faVideoCamera,
  faGamepad,
  faCogs,
  faToggleOff,
  faToggleOn,
  faArrowUp,
  faArrowDown,
  faArrowLeft,
  faArrowRight,
  faSignal,
  faStop,
  faRotateLeft,
  faRotateRight,
} from '@fortawesome/free-solid-svg-icons';
import { faAndroid } from '@fortawesome/free-brands-svg-icons';

interface FunctionStates {
  visualLine: boolean;
  obstacleAvoidance: boolean;
  autoMode: boolean;
  yolo: boolean;
  tracking: boolean;
  horn: boolean;
}

interface ControllerPanelProps {
  onShowStatus: () => void;
}

export default function ControllerPanel({
  onShowStatus,
}: ControllerPanelProps) {
  // 摄像头状态
  const [cameraStatus, setCameraStatus] = useState<"online" | "offline">("offline");
  const [videoUrl, setVideoUrl] = useState<string>('');

  // 添加YOLO相关状态
  const normalVideoUrl = 'http://192.168.5.165:6500/video_feed';
  const yoloVideoUrl = 'http://192.168.5.165:6500/yolo_video_feed';
  const [cameraTitle, setCameraTitle] = useState<string>("实时摄像头");

  // 功能状态
  const [functionStates, setFunctionStates] = useState<FunctionStates>({
    visualLine: false,
    obstacleAvoidance: false,
    autoMode: false,
    yolo: false,
    tracking: false,
    horn: false,
  });

  // 定义命令映射
  const commandMap = {
    visualLine: {
      on: '$01630266#',
      off: '$01640267#'
    },
    obstacleAvoidance: {
      on: '$0170040075#',
      off: '$0171040076#'
    },
    autoMode: {
      on: '$0172040077#',
      off: '$0173040078#'
    },
    tracking: {
      on: '$0174040079#',
      off: '$017504007A#'
    },
    horn: {
      on: '$01130601647F#',
      off: null
    },
  };

  // 切换功能状态
  const toggleFunction = async (func: keyof FunctionStates) => {

    if (func === 'yolo') {
      // YOLO功能特殊处理
      setFunctionStates(prev => {
        const newState = !prev[func];
        // 根据新状态切换视频源和标题
        setVideoUrl(newState ? yoloVideoUrl : normalVideoUrl);
        setCameraTitle(newState ? "实时YOLO检测" : "实时摄像头");
        return {
          ...prev,
          [func]: newState
        };
      });
      return;
    }

    const command = commandMap[func]?.[!functionStates[func] ? 'on' : 'off'];

    if (command) {
      try {
        const response = await fetch('http://192.168.5.141:5000/send-command', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
          },
          body: JSON.stringify({ command }),
        });

        const result = await response.json();
        if (result.success) {
          console.log(`${func} ${!functionStates[func] ? '开启' : '关闭'}指令已发送:`, command);
          // 更新状态 - 对于鸣笛系统特殊处理
          if (func === 'horn') {
            // 先设置为开启状态
            setFunctionStates((prev) => ({
              ...prev,
              [func]: true,
            }));
            // 0.5秒后自动设置回关闭状态
            setTimeout(() => {
              setFunctionStates((prev) => ({
                ...prev,
                [func]: false,
              }));
            }, 500);
          } else {
            // 其他功能正常切换状态
            setFunctionStates((prev) => ({
              ...prev,
              [func]: !prev[func],
            }));
          }
        } else {
          console.error('发送失败:', result.error);
        }
      } catch (error) {
        console.error('网络错误:', error);
      }
    } else if (func === 'horn' && functionStates[func]) {
      // 鸣笛系统从开启变为关闭状态时，直接更新状态，不发送命令
      setFunctionStates((prev) => ({
        ...prev,
        [func]: false,
      }));
    } else {
      // 其他不需要发送命令的功能，直接更新状态
      setFunctionStates((prev) => ({
        ...prev,
        [func]: !prev[func],
      }));
    }

    console.log(`${func} 功能已${!functionStates[func] ? "开启" : "关闭"}`);
  };

  // 处于活跃的按钮状态集合（用于视觉反馈）
  const [activeKeys, setActiveKeys] = useState<Set<string>>(new Set());

  // 检查按钮是否处于激活状态
  const isDirectionActive = useCallback(
    (direction: string) => {
      return activeKeys.has(direction);
    },
    [activeKeys]
  );

  // 组件挂载时的初始化
  useEffect(() => {
    const cameraTimer = setTimeout(() => {
      setCameraStatus("online");
      setVideoUrl('http://192.168.5.165:6500/video_feed');
    }, 2000);

    return () => {
      clearTimeout(cameraTimer);
    };
  }, []);

  return (
    <>
      {/* 头部 */}
      <header className="relative bg-gradient-to-r from-blue-600 to-cyan-400 rounded-2xl p-8 mb-8 shadow-lg overflow-hidden">
        <div className="absolute inset-0 bg-gradient-to-r from-transparent via-white/10 to-transparent animate-header-shine"></div>
        <div className="flex items-center justify-between relative z-10">
          <div>
            <h1 className="text-4xl font-bold text-white flex items-center">
              <FontAwesomeIcon icon={faAndroid} className="mr-4" />{" "}
              机器小车控制面板
            </h1>
            <p className="text-lg mt-2 text-white/90">
              控制机器小车移动和功能的界面
            </p>
          </div>
          <button
            onClick={onShowStatus}
            className="bg-white/20 hover:bg-white/30 text-white px-6 py-3 rounded-xl flex items-center gap-2 transition-all duration-300 border-2 border-white/30 hover:border-white/50"
          >
            <FontAwesomeIcon icon={faSignal} />
            查看状态
          </button>
        </div>
      </header>

      {/* 主要内容 */}
      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8 mb-8">
        {/* 摄像头区域 */}
        <section className="bg-dark-accent rounded-2xl p-6 border-2 border-cyan-400/30 shadow-lg relative overflow-hidden">
          <div className="absolute inset-0 bg-gradient-to-r from-transparent via-cyan-400/5 to-transparent animate-section-glow"></div>
          <h2 className="text-2xl font-semibold flex items-center mb-5 text-light-text relative z-10">
            <FontAwesomeIcon
              icon={faVideoCamera}
              className="mr-3 text-cyan-400"
            />{" "}
            {cameraTitle}
          </h2>

          <div className="relative w-full h-80 bg-black rounded-2xl overflow-hidden border-2 border-cyan-400 shadow-inner">
            <div className="w-full h-full bg-gradient-to-br from-slate-800 to-slate-900 flex items-center justify-center text-light-text text-lg">
              {cameraStatus === "online" ? (
                <img
                  src={videoUrl}
                  alt={cameraTitle}
                  className="w-full h-full object-contain"
                  style={{
                    display: videoUrl ? 'block' : 'none',
                    backgroundColor: 'black'
                  }}
                  onError={() => {
                    console.error('视频流加载失败');
                    setCameraStatus("offline");
                  }}
                />
              ) : (
                "摄像头画面将在这里显示"
              )}
            </div>
            {/* 摄像头状态 */}
            <div
              className={`absolute top-4 right-4 px-3 py-1 rounded-lg text-sm font-bold border transition-all duration-300 ${cameraStatus === "online"
                  ? "bg-black/70 border-success-color text-success-color"
                  : "bg-black/70 border-danger-color text-danger-color"
                }`}
            >
              {cameraStatus === "online" ? "在线" : "离线"}
            </div>
          </div>

        </section>

        {/* 控制区域 */}
        <section className="bg-dark-accent rounded-2xl p-6 border-2 border-orange-400/30 shadow-lg relative overflow-hidden">
          <div className="absolute inset-0 bg-gradient-to-r from-transparent via-orange-400/5 to-transparent animate-section-glow"></div>
          <h2 className="text-2xl font-semibold flex items-center mb-5 text-light-text relative z-10">
            <FontAwesomeIcon
              icon={faGamepad}
              className="mr-3 text-orange-400"
            />{" "}
            方向控制
          </h2>
          <div className="grid grid-cols-3 gap-4 max-w-xs mx-auto relative z-10">

            <div></div>

            {/* 前进按钮 */}
            <button
              onClick={async () => {
                const command = '$011504011B#';
                try {
                  const response = await fetch('http://192.168.5.141:5000/send-command', {
                    method: 'POST',
                    headers: {
                      'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ command }),
                  });

                  const result = await response.json();
                  if (result.success) {
                    console.log('前进指令已发送:', command);
                    setActiveKeys(prev => new Set(prev).add('up'));
                    setTimeout(() => {
                      setActiveKeys(prev => {
                        const newSet = new Set(prev);
                        newSet.delete('up');
                        return newSet;
                      });
                    }, 1000);
                  } else {
                    console.error('发送失败:', result.error);
                  }
                } catch (error) {
                  console.error('网络错误:', error);
                }
              }}
              className={`w-16 h-16 border-2 border-cyan-400/30 bg-gradient-button text-light-text rounded-xl cursor-pointer text-2xl transition-all duration-300 relative overflow-hidden hover:border-cyan-400 hover:shadow-lg hover:shadow-cyan-400/30 active:scale-95 ${isDirectionActive("up")
                ? "bg-cyan-400 border-cyan-400 shadow-lg shadow-cyan-400/50 text-dark-text scale-95 active-border-glow"
                : ""
                }`}
            >
              <FontAwesomeIcon
                icon={faArrowUp}
                className={isDirectionActive("up") ? "float-animation" : ""}
              />
              {isDirectionActive("up") && (
                <div className="absolute inset-0 bg-cyan-400/20 animate-pulse rounded-xl"></div>
              )}
            </button>

            <div></div>

            {/* 向左按钮 */}
            <button
              onClick={async () => {
                const command = '$011504031D#';
                try {
                  const response = await fetch('http://192.168.5.141:5000/send-command', {
                    method: 'POST',
                    headers: {
                      'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ command }),
                  });

                  const result = await response.json();
                  if (result.success) {
                    console.log('左转指令已发送:', command);
                    setActiveKeys(prev => new Set(prev).add('left'));
                    setTimeout(() => {
                      setActiveKeys(prev => {
                        const newSet = new Set(prev);
                        newSet.delete('left');
                        return newSet;
                      });
                    }, 1000);
                  } else {
                    console.error('发送失败:', result.error);
                  }
                } catch (error) {
                  console.error('网络错误:', error);
                }
              }}
              className={`w-16 h-16 border-2 border-cyan-400/30 bg-gradient-button text-light-text rounded-xl cursor-pointer text-2xl transition-all duration-300 relative overflow-hidden hover:border-cyan-400 hover:shadow-lg hover:shadow-cyan-400/30 active:scale-95 ${isDirectionActive("left")
                ? "bg-cyan-400 border-cyan-400 shadow-lg shadow-cyan-400/50 text-dark-text scale-95 active-border-glow"
                : ""
                }`}
            >
              <FontAwesomeIcon
                icon={faArrowLeft}
                className={isDirectionActive("left") ? "float-animation" : ""}
              />
              {isDirectionActive("left") && (
                <div className="absolute inset-0 bg-cyan-400/20 animate-pulse rounded-xl"></div>
              )}
            </button>

            {/* 停止按钮 */}
            <button
              onClick={async () => {
                const command = '$011504001A#';
                try {
                  const response = await fetch('http://192.168.5.141:5000/send-command', {
                    method: 'POST',
                    headers: {
                      'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ command }),
                  });

                  const result = await response.json();
                  if (result.success) {
                    console.log('停止指令已发送:', command);
                    setActiveKeys(new Set());
                  } else {
                    console.error('发送失败:', result.error);
                  }
                } catch (error) {
                  console.error('网络错误:', error);
                }
              }}
              className="w-16 h-16 border-2 border-red-400/30 bg-gradient-button text-light-text rounded-xl cursor-pointer text-2xl transition-all duration-300 relative overflow-hidden hover:border-red-400 hover:shadow-lg hover:shadow-red-400/30 active:scale-95"
            >
              <FontAwesomeIcon icon={faStop} className="text-red-400" />
            </button>

            {/* 向右按钮 */}
            <button
              onClick={async () => {
                const command = '$011504041E#';
                try {
                  const response = await fetch('http://192.168.5.141:5000/send-command', {
                    method: 'POST',
                    headers: {
                      'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ command }),
                  });

                  const result = await response.json();
                  if (result.success) {
                    console.log('右转指令已发送:', command);
                    setActiveKeys(prev => new Set(prev).add('right'));
                    setTimeout(() => {
                      setActiveKeys(prev => {
                        const newSet = new Set(prev);
                        newSet.delete('right');
                        return newSet;
                      });
                    }, 1000);
                  } else {
                    console.error('发送失败:', result.error);
                  }
                } catch (error) {
                  console.error('网络错误:', error);
                }
              }}
              className={`w-16 h-16 border-2 border-cyan-400/30 bg-gradient-button text-light-text rounded-xl cursor-pointer text-2xl transition-all duration-300 relative overflow-hidden hover:border-cyan-400 hover:shadow-lg hover:shadow-cyan-400/30 active:scale-95 ${isDirectionActive("right")
                ? "bg-cyan-400 border-cyan-400 shadow-lg shadow-cyan-400/50 text-dark-text scale-95 active-border-glow"
                : ""
                }`}
            >
              <FontAwesomeIcon
                icon={faArrowRight}
                className={isDirectionActive("right") ? "float-animation" : ""}
              />
              {isDirectionActive("right") && (
                <div className="absolute inset-0 bg-cyan-400/20 animate-pulse rounded-xl"></div>
              )}
            </button>

            {/* 左转按钮 */}
            <button
              onClick={async () => {
                const command = '$011504051F#';
                try {
                  const response = await fetch('http://192.168.5.141:5000/send-command', {
                    method: 'POST',
                    headers: {
                      'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ command }),
                  });

                  const result = await response.json();
                  if (result.success) {
                    console.log('左转旋转指令已发送:', command);
                    setActiveKeys(prev => new Set(prev).add('rotateLeft'));
                    setTimeout(() => {
                      setActiveKeys(prev => {
                        const newSet = new Set(prev);
                        newSet.delete('rotateLeft');
                        return newSet;
                      });
                    }, 1000);
                  } else {
                    console.error('发送失败:', result.error);
                  }
                } catch (error) {
                  console.error('网络错误:', error);
                }
              }}
              className={`w-24 h-12 border-2 border-purple-400/30 bg-gradient-button text-light-text rounded-xl cursor-pointer text-xl transition-all duration-300 relative overflow-hidden hover:border-purple-400 hover:shadow-lg hover:shadow-purple-400/30 active:scale-95 ${isDirectionActive("rotateLeft")
                ? "bg-purple-400 border-purple-400 shadow-lg shadow-purple-400/50 text-dark-text scale-95 active-border-glow"
                : ""
                }`}
            >
              <FontAwesomeIcon
                icon={faRotateLeft}
                className={`text-purple-400 ${isDirectionActive("rotateLeft") ? "float-animation" : ""}`}
              />
              {isDirectionActive("rotateLeft") && (
                <div className="absolute inset-0 bg-purple-400/20 animate-pulse rounded-xl"></div>
              )}
            </button>

            {/* 向后按钮 */}
            <button
              onClick={async () => {
                const command = '$011504021C#';
                try {
                  const response = await fetch('http://192.168.5.141:5000/send-command', {
                    method: 'POST',
                    headers: {
                      'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ command }),
                  });

                  const result = await response.json();
                  if (result.success) {
                    console.log('后退指令已发送:', command);
                    setActiveKeys(prev => new Set(prev).add('down'));
                    setTimeout(() => {
                      setActiveKeys(prev => {
                        const newSet = new Set(prev);
                        newSet.delete('down');
                        return newSet;
                      });
                    }, 1000);
                  } else {
                    console.error('发送失败:', result.error);
                  }
                } catch (error) {
                  console.error('网络错误:', error);
                }
              }}
              className={`w-16 h-16 border-2 border-cyan-400/30 bg-gradient-button text-light-text rounded-xl cursor-pointer text-2xl transition-all duration-300 relative overflow-hidden hover:border-cyan-400 hover:shadow-lg hover:shadow-cyan-400/30 active:scale-95 ${isDirectionActive("down")
                ? "bg-cyan-400 border-cyan-400 shadow-lg shadow-cyan-400/50 text-dark-text scale-95 active-border-glow"
                : ""
                }`}
            >
              <FontAwesomeIcon
                icon={faArrowDown}
                className={isDirectionActive("down") ? "float-animation" : ""}
              />
              {isDirectionActive("down") && (
                <div className="absolute inset-0 bg-cyan-400/20 animate-pulse rounded-xl"></div>
              )}
            </button>

            {/* 右转按钮 */}
            <button
              onClick={async () => {
                const command = '$0115040620#';
                try {
                  const response = await fetch('http://192.168.5.141:5000/send-command', {
                    method: 'POST',
                    headers: {
                      'Content-Type': 'application/json',
                    },
                    body: JSON.stringify({ command }),
                  });

                  const result = await response.json();
                  if (result.success) {
                    console.log('右转旋转指令已发送:', command);
                    setActiveKeys(prev => new Set(prev).add('rotateRight'));
                    setTimeout(() => {
                      setActiveKeys(prev => {
                        const newSet = new Set(prev);
                        newSet.delete('rotateRight');
                        return newSet;
                      });
                    }, 1000);
                  } else {
                    console.error('发送失败:', result.error);
                  }
                } catch (error) {
                  console.error('网络错误:', error);
                }
              }}
              className={`w-24 h-12 border-2 border-purple-400/30 bg-gradient-button text-light-text rounded-xl cursor-pointer text-xl transition-all duration-300 relative overflow-hidden hover:border-purple-400 hover:shadow-lg hover:shadow-purple-400/30 active:scale-95 ${isDirectionActive("rotateRight")
                ? "bg-purple-400 border-purple-400 shadow-lg shadow-purple-400/50 text-dark-text scale-95 active-border-glow"
                : ""
                }`}
            >
              <FontAwesomeIcon
                icon={faRotateRight}
                className={`text-purple-400 ${isDirectionActive("rotateRight") ? "float-animation" : ""}`}
              />
              {isDirectionActive("rotateRight") && (
                <div className="absolute inset-0 bg-purple-400/20 animate-pulse rounded-xl"></div>
              )}
            </button>

          </div>
        </section>
      </div>

      {/* 功能控制区 */}
      <section className="bg-dark-accent rounded-2xl p-6 border-2 border-purple-400/30 shadow-lg relative overflow-hidden mb-8">
        <div className="absolute inset-0 bg-gradient-to-r from-transparent via-purple-400/5 to-transparent animate-section-glow"></div>
        <h2 className="text-2xl font-semibold flex items-center mb-5 text-light-text relative z-10">
          <FontAwesomeIcon icon={faCogs} className="mr-3 text-purple-400" />{" "}
          功能控制
        </h2>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4 relative z-10">

          {/* 红外巡线 */}
          <button
            className={`flex items-center justify-between p-4 rounded-xl border-2 border-cyan-400/30 bg-gradient-button text-light-text cursor-pointer transition-all duration-300 hover:border-cyan-400 hover:shadow-lg hover:shadow-cyan-400/30 active:scale-95 ${functionStates.visualLine
              ? "bg-cyan-400 border-cyan-400 shadow-lg shadow-cyan-400/50 text-dark-text active-border-glow"
              : ""
              }`}
            onClick={() => toggleFunction("visualLine")}
          >
            <span className="font-bold">智能巡线</span>
            <div className="relative">
              <FontAwesomeIcon
                icon={functionStates.visualLine ? faToggleOn : faToggleOff}
                className={`text-3xl transition-all duration-300 ${functionStates.visualLine
                  ? "text-light-text float-animation"
                  : ""
                  }`}
              />
              {functionStates.visualLine && (
                <span className="absolute -top-2 -right-2 w-4 h-4 bg-green-500 rounded-full shadow-lg shadow-green-500/50 animate-pulse"></span>
              )}
            </div>
          </button>

          {/* 雷达避障 */}
          <button
            className={`flex items-center justify-between p-4 rounded-xl border-2 border-cyan-400/30 bg-gradient-button text-light-text cursor-pointer transition-all duration-300 hover:border-cyan-400 hover:shadow-lg hover:shadow-cyan-400/30 active:scale-95 ${functionStates.obstacleAvoidance
              ? "bg-cyan-400 border-cyan-400 shadow-lg shadow-cyan-400/50 text-dark-text active-border-glow"
              : ""
              }`}
            onClick={() => toggleFunction("obstacleAvoidance")}
          >
            <span className="font-bold">智能避障</span>
            <div className="relative">
              <FontAwesomeIcon
                icon={
                  functionStates.obstacleAvoidance ? faToggleOn : faToggleOff
                }
                className={`text-3xl transition-all duration-300 ${functionStates.obstacleAvoidance
                  ? "text-light-text float-animation"
                  : ""
                  }`}
              />
              {functionStates.obstacleAvoidance && (
                <span className="absolute -top-2 -right-2 w-4 h-4 bg-green-500 rounded-full shadow-lg shadow-green-500/50 animate-pulse"></span>
              )}
            </div>
          </button>

          {/* 雷达警戒 */}
          <button
            className={`flex items-center justify-between p-4 rounded-xl border-2 border-cyan-400/30 bg-gradient-button text-light-text cursor-pointer transition-all duration-300 hover:border-cyan-400 hover:shadow-lg hover:shadow-cyan-400/30 active:scale-95 ${functionStates.autoMode
              ? "bg-cyan-400 border-cyan-400 shadow-lg shadow-cyan-400/50 text-dark-text active-border-glow"
              : ""
              }`}
            onClick={() => toggleFunction("autoMode")}
          >
            <span className="font-bold">智能警戒</span>
            <div className="relative">
              <FontAwesomeIcon
                icon={functionStates.autoMode ? faToggleOn : faToggleOff}
                className={`text-3xl transition-all duration-300 ${functionStates.autoMode
                  ? "text-light-text float-animation"
                  : ""
                  }`}
              />
              {functionStates.autoMode && (
                <span className="absolute -top-2 -right-2 w-4 h-4 bg-green-500 rounded-full shadow-lg shadow-green-500/50 animate-pulse"></span>
              )}
            </div>
          </button>

          {/* 雷达追踪 */}
          <button
            className={`flex items-center justify-between p-4 rounded-xl border-2 border-cyan-400/30 bg-gradient-button text-light-text cursor-pointer transition-all duration-300 hover:border-cyan-400 hover:shadow-lg hover:shadow-cyan-400/30 active:scale-95 ${functionStates.tracking
              ? "bg-cyan-400 border-cyan-400 shadow-lg shadow-cyan-400/50 text-dark-text active-border-glow"
              : ""
              }`}
            onClick={() => toggleFunction("tracking")}
          >
            <span className="font-bold">智能追踪</span>
            <div className="relative">
              <FontAwesomeIcon
                icon={functionStates.tracking ? faToggleOn : faToggleOff}
                className={`text-3xl transition-all duration-300 ${functionStates.tracking
                  ? "text-light-text float-animation"
                  : ""
                  }`}
              />
              {functionStates.tracking && (
                <span className="absolute -top-2 -right-2 w-4 h-4 bg-green-500 rounded-full shadow-lg shadow-green-500/50 animate-pulse"></span>
              )}
            </div>
          </button>

          {/* YOLO检测 */}
          <button
            className={`flex items-center justify-between p-4 rounded-xl border-2 border-cyan-400/30 bg-gradient-button text-light-text cursor-pointer transition-all duration-300 hover:border-cyan-400 hover:shadow-lg hover:shadow-cyan-400/30 active:scale-95 ${functionStates.yolo
              ? "bg-cyan-400 border-cyan-400 shadow-lg shadow-cyan-400/50 text-dark-text active-border-glow"
              : ""
              }`}
            onClick={() => toggleFunction("yolo")}
          >
            <span className="font-bold">YOLO检测</span>
            <div className="relative">
              <FontAwesomeIcon
                icon={functionStates.yolo ? faToggleOn : faToggleOff}
                className={`text-3xl transition-all duration-300 ${functionStates.yolo ? "text-light-text float-animation" : ""
                  }`}
              />
              {functionStates.yolo && (
                <span className="absolute -top-2 -right-2 w-4 h-4 bg-green-500 rounded-full shadow-lg shadow-green-500/50 animate-pulse"></span>
              )}
            </div>
          </button>

          {/* 鸣笛系统 */}
          <button
            className={`flex items-center justify-between p-4 rounded-xl border-2 border-cyan-400/30 bg-gradient-button text-light-text cursor-pointer transition-all duration-300 hover:border-cyan-400 hover:shadow-lg hover:shadow-cyan-400/30 active:scale-95 ${functionStates.horn
              ? "bg-cyan-400 border-cyan-400 shadow-lg shadow-cyan-400/50 text-dark-text active-border-glow"
              : ""
              }`}
            onClick={() => toggleFunction("horn")}
          >
            <span className="font-bold">鸣笛系统</span>
            <div className="relative">
              <FontAwesomeIcon
                icon={functionStates.horn ? faToggleOn : faToggleOff}
                className={`text-3xl transition-all duration-300 ${functionStates.horn
                  ? "text-light-text float-animation"
                  : ""
                  }`}
              />
              {functionStates.horn && (
                <span className="absolute -top-2 -right-2 w-4 h-4 bg-green-500 rounded-full shadow-lg shadow-green-500/50 animate-pulse"></span>
              )}
            </div>
          </button>

        </div>
      </section>
    </>
  );
}