import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import {
  faArrowLeft,
  faThermometerHalf,
  faTint,
  faLightbulb,
  faSmog,
  faCompress,
  faMapMarkerAlt,
  faBatteryFull,
  faCloudSun,
  faWifi,
  faSignal,
} from '@fortawesome/free-solid-svg-icons';
import { useState, useEffect } from 'react';

interface SensorData {
  temperature: number;
  humidity: number;
  illumination: number;
  smoke: number;
  pressure: number;
  longitude: number;
  latitude: number;
  battery: number;
  pm25: number;
}

interface StatusPanelProps {
  onClose: () => void;
}

export default function StatusPanel({ onClose }: StatusPanelProps) {
  const [sensorData, setSensorData] = useState<SensorData>({
    temperature: 0,
    humidity: 0,
    illumination: 0,
    smoke: 0,
    pressure: 0,
    longitude: 116.3974,
    latitude: 39.9093,
    battery: 0,
    pm25: 0,
  });

  const [lastUpdate, setLastUpdate] = useState<string>(new Date().toLocaleString('zh-CN'));
  const [isConnected, setIsConnected] = useState(false);

  // 传感器数据更新
  useEffect(() => {
    const fetchData = async () => {
      try {
        const response = await fetch('http://localhost:5050/sensor-data');
        if (response.ok) {
          const data = await response.json();
          if (data.error) {
            console.log('暂无数据');
            return;
          }
          setSensorData(data);
          setLastUpdate(data.timestamp || new Date().toLocaleString('zh-CN'));
          setIsConnected(true);
        }
      } catch (error) {
        console.error('获取传感器数据失败:', error);
        setIsConnected(false);
      }
    };

    fetchData();

    const interval = setInterval(fetchData, 100);

    return () => clearInterval(interval);
  }, []);


  const getSensorStatus = (value: number, thresholds: { warning: number; danger: number }) => {
    if (value >= thresholds.danger) return 'danger';
    if (value >= thresholds.warning) return 'warning';
    return 'normal';
  };

  const getStatusColor = (status: string) => {
    switch (status) {
      case 'danger':
        return 'text-red-400 border-red-400';
      case 'warning':
        return 'text-yellow-400 border-yellow-400';
      default:
        return 'text-cyan-400 border-cyan-400';
    }
  };

  const sensorItems = [
    {
      key: 'temperature',
      name: '温度传感器',
      icon: faThermometerHalf,
      value: sensorData.temperature,
      unit: '°C',
      thresholds: { warning: 30, danger: 40 },
    },
    {
      key: 'humidity',
      name: '湿度传感器',
      icon: faTint,
      value: sensorData.humidity,
      unit: '%',
      thresholds: { warning: 70, danger: 85 },
    },
    {
      key: 'illumination',
      name: '光照传感器',
      icon: faLightbulb,
      value: sensorData.illumination,
      unit: 'lux',
      thresholds: { warning: 800, danger: 1000 },
    },
    {
      key: 'smoke',
      name: '烟雾传感器',
      icon: faSmog,
      value: sensorData.smoke,
      unit: 'ppm',
      thresholds: { warning: 50, danger: 80 },
    },
    {
      key: 'pressure',
      name: '压力传感器',
      icon: faCompress,
      value: sensorData.pressure,
      unit: 'Pa',
      thresholds: { warning: 1200, danger: 1500 },
    },
    {
      key: 'longitude',
      name: 'GPS经度',
      icon: faMapMarkerAlt,
      value: sensorData.longitude,
      unit: '°',
      thresholds: { warning: 999, danger: 999 },
      precision: 4,
    },
    {
      key: 'latitude',
      name: 'GPS纬度',
      icon: faMapMarkerAlt,
      value: sensorData.latitude,
      unit: '°',
      thresholds: { warning: 999, danger: 999 },
      precision: 4,
    },
    {
      key: 'battery',
      name: '电池',
      icon: faBatteryFull,
      value: sensorData.battery,
      unit: '%',
      thresholds: { warning: 30, danger: 15 },
      reverse: true,
    },
    {
      key: 'pm25',
      name: 'PM2.5传感器',
      icon: faCloudSun,
      value: sensorData.pm25,
      unit: 'μg/m³',
      thresholds: { warning: 75, danger: 150 },
    },
  ];

  return (
    <>
      {/* 头部 */}
      <header className="relative bg-gradient-to-r from-purple-600 to-blue-400 rounded-2xl p-8 mb-8 shadow-lg overflow-hidden">
        <div className="absolute inset-0 bg-gradient-to-r from-transparent via-white/10 to-transparent animate-header-shine"></div>
        <div className="flex items-center justify-between relative z-10">
          <div className="flex items-center">
            <FontAwesomeIcon icon={faSignal} className="mr-4 text-white text-3xl" />
            <div>
              <h1 className="text-4xl font-bold text-white">传感器状态面板</h1>
              <p className="text-lg mt-2 text-white/90">实时监控机器小车各传感器状态</p>
            </div>
          </div>
          <button
            onClick={onClose}
            className="bg-white/20 hover:bg-white/30 text-white px-6 py-3 rounded-xl flex items-center gap-2 transition-all duration-300 border-2 border-white/30 hover:border-white/50"
          >
            <FontAwesomeIcon icon={faArrowLeft} />
            返回控制面板
          </button>
        </div>
      </header>

      {/* 最新更新时间 */}
      <div className="bg-dark-accent rounded-2xl p-4 mb-6 border-2 border-cyan-400/30">
        <div className="flex items-center justify-between text-light-text">
          <span className="flex items-center gap-2">
            <FontAwesomeIcon
              icon={faWifi}
              className={isConnected ? "text-green-400" : "text-red-400"}
            />
            <span>最新更新时间:</span>
          </span>
          <span className="text-cyan-400 font-bold">{lastUpdate}</span>
        </div>
      </div>

      {/* 传感器状态网格 */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
        {sensorItems.map((sensor) => {
          const status = sensor.reverse
            ? getSensorStatus(100 - sensor.value, sensor.thresholds)
            : getSensorStatus(sensor.value, sensor.thresholds);
          const colorClass = getStatusColor(status);

          return (
            <div
              key={sensor.key}
              className={`bg-dark-accent rounded-2xl p-6 border-2 ${colorClass} shadow-lg relative overflow-hidden group hover:shadow-xl transition-all duration-300`}
            >
              <div className="absolute inset-0 bg-gradient-to-r from-transparent via-cyan-400/5 to-transparent opacity-0 group-hover:opacity-100 transition-opacity duration-300"></div>

              <div className="relative z-10">
                <div className="flex items-center justify-between mb-4">
                  <h3 className="text-lg font-semibold text-light-text">{sensor.name}</h3>
                  <FontAwesomeIcon
                    icon={sensor.icon}
                    className={`text-2xl ${colorClass.split(' ')[0]}`}
                  />
                </div>

                <div className="text-center">
                  <div className={`text-4xl font-bold ${colorClass.split(' ')[0]} mb-2`}>
                    {sensor.precision ? sensor.value.toFixed(sensor.precision) : sensor.value}
                  </div>
                  <div className="text-light-text/70 text-sm">{sensor.unit}</div>
                </div>

                {/* 状态指示器 */}
                <div className="mt-4 flex justify-center">
                  <div
                    className={`w-3 h-3 rounded-full ${status === 'danger'
                      ? 'bg-red-500'
                      : status === 'warning'
                        ? 'bg-yellow-500'
                        : 'bg-green-500'
                      } animate-pulse`}
                  ></div>
                </div>
              </div>
            </div>
          );
        })}
      </div>

      {/* 状态统计 */}
      <div className="mt-8 bg-dark-accent rounded-2xl p-6 border-2 border-cyan-400/30">
        <h3 className="text-2xl font-semibold text-light-text mb-4 flex items-center gap-2">
          <FontAwesomeIcon icon={faSignal} className="text-cyan-400" />
          系统状态统计
        </h3>
        <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
          <div className="text-center p-4 bg-green-500/10 rounded-xl border border-green-500/30">
            <div className="text-2xl font-bold text-green-400">
              {
                sensorItems.filter((s) => {
                  const status = s.reverse
                    ? getSensorStatus(100 - s.value, s.thresholds)
                    : getSensorStatus(s.value, s.thresholds);
                  return status === 'normal';
                }).length
              }
            </div>
            <div className="text-green-400/80 text-sm">正常</div>
          </div>
          <div className="text-center p-4 bg-yellow-500/10 rounded-xl border border-yellow-500/30">
            <div className="text-2xl font-bold text-yellow-400">
              {
                sensorItems.filter((s) => {
                  const status = s.reverse
                    ? getSensorStatus(100 - s.value, s.thresholds)
                    : getSensorStatus(s.value, s.thresholds);
                  return status === 'warning';
                }).length
              }
            </div>
            <div className="text-yellow-400/80 text-sm">警告</div>
          </div>
          <div className="text-center p-4 bg-red-500/10 rounded-xl border border-red-500/30">
            <div className="text-2xl font-bold text-red-400">
              {
                sensorItems.filter((s) => {
                  const status = s.reverse
                    ? getSensorStatus(100 - s.value, s.thresholds)
                    : getSensorStatus(s.value, s.thresholds);
                  return status === 'danger';
                }).length
              }
            </div>
            <div className="text-red-400/80 text-sm">危险</div>
          </div>
        </div>
      </div>
    </>
  );
}
