const char index_html[] = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset='UTF-8'>
  <meta name='viewport' content='width=device-width, initial-scale=1, maximum-scale=1, user-scalable=yes'>
  <title>AI 大棚监控系统 · 优化版</title>
  <script src="https://cdn.tailwindcss.com"></script>
  <script src="https://cdn.jsdelivr.net/npm/echarts@5.4.3/dist/echarts.min.js"></script>
  <style>
    .toggle-switch:checked + div { background-color: #22c55e; }
    .toggle-switch:disabled + div { opacity: 0.5; cursor: not-allowed; }
    .status-dot { display: inline-block; width: 10px; height: 10px; border-radius: 50%; margin-right: 6px; }
    .status-connected { background-color: #22c55e; box-shadow: 0 0 8px #22c55e; }
    .status-disconnected { background-color: #ef4444; box-shadow: 0 0 8px #ef4444; }
    .status-connecting { background-color: #f59e0b; box-shadow: 0 0 8px #f59e0b; }
    .text-quality-excellent { color: #22c55e; }
    .text-quality-good { color: #84cc16; }
    .text-quality-fair { color: #f59e0b; }
    .text-quality-poor { color: #ef4444; }
    .status-badge { display: inline-block; padding: 1px 8px; border-radius: 9999px; font-size: 11px; font-weight: 600; }
    .badge-normal { background: #dcfce7; color: #16a34a; }
    .badge-high   { background: #fee2e2; color: #dc2626; }
    .badge-low    { background: #dbeafe; color: #2563eb; }
    .badge-warn   { background: #fef9c3; color: #ca8a04; }
    .alert-warn   { background: #fffbeb; border-left: 4px solid #f59e0b; padding: 8px 12px; border-radius: 4px; }
    .alert-danger { background: #fef2f2; border-left: 4px solid #ef4444; padding: 8px 12px; border-radius: 4px; }
  </style>
</head>
<body class="bg-slate-100 font-sans p-4 md:p-8 transition-colors duration-200">

  <div class="max-w-6xl mx-auto space-y-6">

    <!-- 标题栏 -->
    <div class="flex flex-col md:flex-row items-center justify-between bg-white rounded-2xl shadow-sm p-6">
      <div class="flex items-center space-x-3">
        <h1 class="text-3xl font-extrabold text-green-700 tracking-tight">🤖 AI 大棚核心控制</h1>
        <div id="connection-status" class="flex items-center text-sm font-medium text-gray-600 bg-gray-100 px-3 py-1 rounded-full">
          <span class="status-dot status-connecting" id="status-dot"></span>
          <span id="status-text">连接中...</span>
        </div>
      </div>
      <a href='/reset' class="mt-4 md:mt-0 px-4 py-2 bg-red-500 hover:bg-red-600 text-white rounded-lg shadow transition-colors text-sm font-medium" onclick="return confirm('确定要清除 WiFi 配置并重启吗？')">⚠️ 重置WiFi</a>
    </div>

    <!-- 👇 修改：实时环境数据卡片扩容为 6 项 -->
    <div class="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-6 gap-4">
      <div class="bg-white p-4 rounded-2xl shadow-sm text-center flex flex-col items-center justify-center hover:shadow-md transition-shadow">
        <div class="text-gray-500 text-xs font-semibold mb-1">🌡️ 温度</div>
        <div class="text-2xl font-bold text-red-500" id="val-temp">-- °C</div>
        <div class="mt-1"><span class="status-badge" id="badge-temp">--</span></div>
      </div>
      <div class="bg-white p-4 rounded-2xl shadow-sm text-center flex flex-col items-center justify-center hover:shadow-md transition-shadow">
        <div class="text-gray-500 text-xs font-semibold mb-1">💧 湿度</div>
        <div class="text-2xl font-bold text-blue-500" id="val-hum">-- %</div>
        <div class="mt-1"><span class="status-badge" id="badge-hum">--</span></div>
      </div>
      <div class="bg-white p-4 rounded-2xl shadow-sm text-center flex flex-col items-center justify-center hover:shadow-md transition-shadow">
        <div class="text-gray-500 text-xs font-semibold mb-1">☀️ 光照</div>
        <div class="text-2xl font-bold text-orange-400" id="val-lux">-- lx</div>
        <div class="mt-1"><span class="status-badge" id="badge-lux">--</span></div>
      </div>
      <div class="bg-white p-4 rounded-2xl shadow-sm text-center flex flex-col items-center justify-center hover:shadow-md transition-shadow">
        <div class="text-gray-500 text-xs font-semibold mb-1">🌱 土壤</div>
        <div class="text-2xl font-bold text-green-600" id="val-soil">-- %</div>
        <div class="mt-1"><span class="status-badge" id="badge-soil">--</span></div>
      </div>
      <div class="bg-white p-4 rounded-2xl shadow-sm text-center flex flex-col items-center justify-center hover:shadow-md transition-shadow">
        <div class="text-gray-500 text-xs font-semibold mb-1">☁️ CO2 (eCO2)</div>
        <div class="text-2xl font-bold text-teal-500" id="val-eco2">-- ppm</div>
        <div class="mt-1"><span class="status-badge" id="badge-eco2">--</span></div>
      </div>
      <div class="bg-white p-4 rounded-2xl shadow-sm text-center flex flex-col items-center justify-center hover:shadow-md transition-shadow">
        <div class="text-gray-500 text-xs font-semibold mb-1">🧪 有机挥发物</div>
        <div class="text-2xl font-bold text-purple-500" id="val-tvoc">-- ppb</div>
        <div class="mt-1"><span class="status-badge" id="badge-tvoc">--</span></div>
      </div>
    </div>

    <!-- 系统状态卡片 -->
    <div class="bg-white rounded-2xl shadow-sm p-6">
      <h2 class="text-xl font-bold text-gray-800 mb-4 border-b pb-3">⚙️ 系统状态</h2>
      <div class="grid grid-cols-2 md:grid-cols-4 gap-x-6 gap-y-4 text-sm">
        <div class="flex justify-between border-b pb-1">
          <span class="font-semibold text-gray-600">内存占用:</span>
          <span class="font-mono text-gray-800 font-bold" id="val-heap">-- %</span>
        </div>
        <div class="flex justify-between border-b pb-1">
          <span class="font-semibold text-gray-600">WiFi信号:</span>
          <span class="font-mono text-gray-800 font-bold" id="val-rssi">-- dBm</span>
        </div>
        <div class="flex justify-between border-b pb-1">
          <span class="font-semibold text-gray-600">信号质量:</span>
          <span class="font-bold" id="val-rssi-quality">--</span>
        </div>
        <div class="flex justify-between border-b pb-1">
          <span class="font-semibold text-gray-600">芯片版本:</span>
          <span class="font-mono text-gray-800 font-bold" id="val-chip-rev">--</span>
        </div>
        <div class="flex justify-between border-b pb-1 col-span-2 md:col-span-4">
          <span class="font-semibold text-gray-600">MAC地址:</span>
          <span class="font-mono text-gray-800 font-bold" id="val-mac">--:--:--:--:--:--</span>
        </div>
        <div class="flex justify-between border-b pb-1">
          <span class="font-semibold text-gray-600">IP地址:</span>
          <span class="font-mono text-gray-800 font-bold" id="val-ip">--</span>
        </div>
        <div class="flex justify-between border-b pb-1">
          <span class="font-semibold text-gray-600">CPU频率:</span>
          <span class="font-mono text-gray-800 font-bold" id="val-cpu-freq">-- MHz</span>
        </div>
        <div class="flex justify-between border-b pb-1">
          <span class="font-semibold text-gray-600">CPU核心:</span>
          <span class="font-mono text-gray-800 font-bold" id="val-cpu-cores">--</span>
        </div>
        <div class="flex justify-between border-b pb-1">
          <span class="font-semibold text-gray-600">运行时间:</span>
          <span class="font-mono text-gray-800 font-bold" id="val-uptime">--</span>
        </div>
        <div class="flex justify-between border-b pb-1 col-span-2 md:col-span-4">
          <span class="font-semibold text-gray-600">SDK版本:</span>
          <span class="font-mono text-gray-800 text-xs font-bold" id="val-sdk">--</span>
        </div>
      </div>
    </div>

    <!-- 设备控制面板 -->
    <div class="bg-white rounded-2xl shadow-sm p-6">
      <div class="flex items-center justify-between mb-6 border-b pb-4">
        <h2 class="text-xl font-bold text-gray-800">🎛️ 设备控制面板</h2>
        <label class="relative inline-flex items-center cursor-pointer">
          <input type="checkbox" id="cb-mode" class="sr-only peer toggle-switch" onchange="toggleMode()">
          <div class="w-14 h-7 bg-gray-300 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-6 after:w-6 after:transition-all"></div>
          <span class="ml-3 font-bold text-lg w-28 text-left" id="mode-text">获取中...</span>
        </label>
      </div>

      <div class="grid grid-cols-2 md:grid-cols-4 gap-4">
        <div class="flex flex-col items-center p-4 bg-slate-50 rounded-xl border border-slate-100">
          <span class="font-bold text-gray-700 mb-3 text-lg">💦 水泵</span>
          <label class="relative inline-flex items-center cursor-pointer">
            <input type="checkbox" id="cb-pump" class="sr-only peer toggle-switch" onchange="toggleDevice('pump')">
            <div class="w-12 h-6 bg-gray-300 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all"></div>
          </label>
        </div>
        <div class="flex flex-col items-center p-4 bg-slate-50 rounded-xl border border-slate-100">
          <span class="font-bold text-gray-700 mb-3 text-lg">💡 补光灯</span>
          <label class="relative inline-flex items-center cursor-pointer">
            <input type="checkbox" id="cb-light" class="sr-only peer toggle-switch" onchange="toggleDevice('light')">
            <div class="w-12 h-6 bg-gray-300 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all"></div>
          </label>
        </div>
        <div class="flex flex-col items-center p-4 bg-slate-50 rounded-xl border border-slate-100">
          <span class="font-bold text-gray-700 mb-3 text-lg">🔥 加热垫</span>
          <label class="relative inline-flex items-center cursor-pointer">
            <input type="checkbox" id="cb-heater" class="sr-only peer toggle-switch" onchange="toggleDevice('heater')">
            <div class="w-12 h-6 bg-gray-300 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all"></div>
          </label>
        </div>
        <div class="flex flex-col items-center p-4 bg-slate-50 rounded-xl border border-slate-100">
          <span class="font-bold text-gray-700 mb-3 text-lg">🌬️ 风扇</span>
          <label class="relative inline-flex items-center cursor-pointer">
            <input type="checkbox" id="cb-fan" class="sr-only peer toggle-switch" onchange="toggleDevice('fan')">
            <div class="w-12 h-6 bg-gray-300 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-5 after:w-5 after:transition-all"></div>
          </label>
        </div>
      </div>
    </div>

    <!-- 环境预警 -->
    <div class="bg-white rounded-2xl shadow-sm p-6">
      <h2 class="text-xl font-bold text-gray-800 mb-4 border-b pb-3">🔔 环境预警</h2>
      <div id="alert-list" class="space-y-2 text-sm"><div class="text-gray-400">等待数据...</div></div>
    </div>

    <!-- 历史统计摘要 -->
    <div class="bg-white rounded-2xl shadow-sm p-6">
      <h2 class="text-xl font-bold text-gray-800 mb-4 border-b pb-3">📊 历史统计摘要（近60条）</h2>
      <div class="overflow-x-auto">
        <table class="min-w-full text-sm text-center">
          <thead>
            <tr class="bg-slate-50 text-gray-600">
              <th class="px-3 py-2 font-semibold text-left">指标</th>
              <th class="px-3 py-2 font-semibold">最小值</th>
              <th class="px-3 py-2 font-semibold">最大值</th>
              <th class="px-3 py-2 font-semibold">平均值</th>
              <th class="px-3 py-2 font-semibold">当前值</th>
            </tr>
          </thead>
          <tbody id="stats-body">
            <tr><td colspan="5" class="py-4 text-gray-400">等待历史数据...</td></tr>
          </tbody>
        </table>
      </div>
    </div>

    <!-- 任务调度与定时任务面板 -->
    <div class="bg-white rounded-2xl shadow-sm p-6 overflow-hidden">
      <div class="flex items-center justify-between mb-4 border-b pb-3">
        <h2 class="text-xl font-bold text-gray-800 flex items-center">
          <span class="mr-2">📅</span> 定时任务调度器
        </h2>
        <span class="text-xs font-mono text-gray-400" id="val-time-sync">时间同步中...</span>
      </div>
      <div class="grid grid-cols-1 md:grid-cols-2 gap-6">
        <!-- 抽风/风扇定时 -->
        <div class="p-4 bg-slate-50 rounded-xl border border-slate-100">
          <div class="flex justify-between items-center mb-3">
            <span class="font-bold text-gray-700">🌬️ 强制通风 (每日)</span>
            <label class="relative inline-flex items-center cursor-pointer">
              <input type="checkbox" id="sched-fan-en" class="sr-only peer toggle-switch" onchange="saveSchedule()">
              <div class="w-10 h-5 bg-gray-300 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-4 after:w-4 after:transition-all"></div>
            </label>
          </div>
          <div class="flex items-center space-x-2 text-sm text-gray-600">
            <input type="time" id="sched-fan-start" class="px-2 py-1 rounded border border-gray-200 focus:ring-1 focus:ring-blue-400 outline-none" onchange="saveSchedule()">
            <span>至</span>
            <input type="time" id="sched-fan-end" class="px-2 py-1 rounded border border-gray-200 focus:ring-1 focus:ring-blue-400 outline-none" onchange="saveSchedule()">
          </div>
        </div>
        <!-- 补光定时 -->
        <div class="p-4 bg-slate-50 rounded-xl border border-slate-100">
          <div class="flex justify-between items-center mb-3">
            <span class="font-bold text-gray-700">💡 补光计划 (每日)</span>
            <label class="relative inline-flex items-center cursor-pointer">
              <input type="checkbox" id="sched-light-en" class="sr-only peer toggle-switch" onchange="saveSchedule()">
              <div class="w-10 h-5 bg-gray-300 peer-focus:outline-none rounded-full peer peer-checked:after:translate-x-full peer-checked:after:border-white after:content-[''] after:absolute after:top-[2px] after:left-[2px] after:bg-white after:border-gray-300 after:border after:rounded-full after:h-4 after:w-4 after:transition-all"></div>
            </label>
          </div>
          <div class="flex items-center space-x-2 text-sm text-gray-600">
            <input type="time" id="sched-light-start" class="px-2 py-1 rounded border border-gray-200 focus:ring-1 focus:ring-blue-400 outline-none" onchange="saveSchedule()">
            <span>至</span>
            <input type="time" id="sched-light-end" class="px-2 py-1 rounded border border-gray-200 focus:ring-1 focus:ring-blue-400 outline-none" onchange="saveSchedule()">
          </div>
        </div>
      </div>
      <div class="mt-4 text-xs text-gray-400 bg-blue-50 p-2 rounded-lg">
        * 任务在“自动模式”下生效。手动模式将忽略定时冲突。系统时间由 NTP 自动同步。
      </div>
    </div>

    <!-- 👇 修改：历史曲线 (变更为 3 个图表排排坐) -->
    <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">
      <div class="bg-white rounded-2xl shadow-sm p-4 h-80 w-full" id="chart-th"></div>
      <div class="bg-white rounded-2xl shadow-sm p-4 h-80 w-full" id="chart-ls"></div>
      <div class="bg-white rounded-2xl shadow-sm p-4 h-80 w-full" id="chart-aq"></div>
    </div>
  </div>

  <script>
    (function() {
      // ----- 初始化图表 -----
      var chartTH = echarts.init(document.getElementById('chart-th'));
      var chartLS = echarts.init(document.getElementById('chart-ls'));
      var chartAQ = echarts.init(document.getElementById('chart-aq')); // 👇 新增图表

      var optTH = {
        title: { text: '温湿度历史', left: 'center', textStyle: { color: '#475569', fontSize: 14 } },
        tooltip: { trigger: 'axis' },
        legend: { data: ['温度(°C)', '湿度(%)'], top: 25, itemWidth: 10, itemHeight: 10 },
        grid: { left: '15%', right: '15%', bottom: '10%', top: '25%' },
        xAxis: { type: 'category', data: [] },
        yAxis:[
          { type: 'value', name: '°C' },
          { type: 'value', name: '%', position: 'right', min: 0, max: 100 }
        ],
        series:[
          { name: '温度(°C)', type: 'line', data: [], smooth: true, itemStyle: { color: '#ef4444' }, areaStyle: { opacity: 0.1 } },
          { name: '湿度(%)', type: 'line', yAxisIndex: 1, data:[], smooth: true, itemStyle: { color: '#3b82f6' }, areaStyle: { opacity: 0.1 } }
        ]
      };
      
      var optLS = {
        title: { text: '光照与土壤水分', left: 'center', textStyle: { color: '#475569', fontSize: 14 } },
        tooltip: { trigger: 'axis' },
        legend: { data: ['光照(lx)', '土壤(%)'], top: 25, itemWidth: 10, itemHeight: 10 },
        grid: { left: '15%', right: '15%', bottom: '10%', top: '25%' },
        xAxis: { type: 'category', data: [] },
        yAxis:[
          { type: 'value', name: 'lx' },
          { type: 'value', name: '%', position: 'right', min: 0, max: 100 }
        ],
        series:[
          { name: '光照(lx)', type: 'line', data: [], smooth: true, itemStyle: { color: '#f97316' }, areaStyle: { opacity: 0.1 } },
          { name: '土壤(%)', type: 'line', yAxisIndex: 1, data:[], smooth: true, itemStyle: { color: '#22c55e' }, areaStyle: { opacity: 0.1 } }
        ]
      };

      // 👇 新增空气质量图表配置
      var optAQ = {
        title: { text: '空气质量 (SGP30)', left: 'center', textStyle: { color: '#475569', fontSize: 14 } },
        tooltip: { trigger: 'axis' },
        legend: { data:['eCO2(ppm)', 'TVOC(ppb)'], top: 25, itemWidth: 10, itemHeight: 10 },
        grid: { left: '15%', right: '15%', bottom: '10%', top: '25%' },
        xAxis: { type: 'category', data:[] },
        yAxis:[
          { type: 'value', name: 'ppm', min: 400 },
          { type: 'value', name: 'ppb', position: 'right' }
        ],
        series:[
          { name: 'eCO2(ppm)', type: 'line', data: [], smooth: true, itemStyle: { color: '#14b8a6' }, areaStyle: { opacity: 0.1 } },
          { name: 'TVOC(ppb)', type: 'line', yAxisIndex: 1, data:[], smooth: true, itemStyle: { color: '#a855f7' }, areaStyle: { opacity: 0.1 } }
        ]
      };

      chartTH.setOption(optTH);
      chartLS.setOption(optLS);
      chartAQ.setOption(optAQ); // 挂载新图表

      // ----- WebSocket 管理 -----
      var gateway = `ws://${window.location.hostname}:81/`;
      var socket = null;
      var reconnectTimer = null;

      function setConnectionStatus(state) {
        var dot = document.getElementById('status-dot');
        var text = document.getElementById('status-text');
        dot.classList.remove('status-connected', 'status-disconnected', 'status-connecting');
        if (state === 'connected') {
          dot.classList.add('status-connected'); text.innerText = '已连接';
        } else if (state === 'disconnected') {
          dot.classList.add('status-disconnected'); text.innerText = '未连接';
        } else {
          dot.classList.add('status-connecting'); text.innerText = '连接中...';
        }
      }

      function connectWebSocket() {
        if (reconnectTimer) clearTimeout(reconnectTimer);
        if (socket && socket.readyState !== WebSocket.CLOSED) socket.close();
        setConnectionStatus('connecting');
        socket = new WebSocket(gateway);

        socket.onopen = function() { setConnectionStatus('connected'); };
        socket.onmessage = function(event) { updateUI(JSON.parse(event.data)); };
        socket.onerror = function() { setConnectionStatus('disconnected'); };
        socket.onclose = function() {
          setConnectionStatus('disconnected');
          reconnectTimer = setTimeout(connectWebSocket, 3000);
        };
      }
      connectWebSocket();
      document.addEventListener('visibilitychange', function() {
        if (!document.hidden && (!socket || socket.readyState === WebSocket.CLOSED)) connectWebSocket();
      });

      // ----- 工具函数 -----
      function setSensorBadge(id, val, lowT, warnHigh, highT) {
        var el = document.getElementById(id);
        if (!el) return;
        if (val === undefined || val === null) { el.className = 'status-badge'; el.innerText = '--'; return; }
        if (val < lowT)       { el.className = 'status-badge badge-low';    el.innerText = '偏低'; }
        else if (val > highT) { el.className = 'status-badge badge-high';   el.innerText = '偏高'; }
        else if (val > warnHigh) { el.className = 'status-badge badge-warn'; el.innerText = '注意'; }
        else                  { el.className = 'status-badge badge-normal'; el.innerText = '正常'; }
      }

      function formatUptime(sec) {
        var d = Math.floor(sec / 86400), h = Math.floor((sec % 86400) / 3600),
            m = Math.floor((sec % 3600) / 60), s = sec % 60;
        if (d > 0) return d + '天 ' + h + '时 ' + m + '分';
        if (h > 0) return h + '时 ' + m + '分 ' + s + '秒';
        return m + '分 ' + s + '秒';
      }

      function calcStats(arr) {
        if (!arr || arr.length === 0) return null;
        var min = Infinity, max = -Infinity, sum = 0;
        for (var i = 0; i < arr.length; i++) {
          var v = parseFloat(arr[i]);
          if (v < min) min = v; if (v > max) max = v; sum += v;
        }
        return { min: min, max: max, avg: sum / arr.length };
      }

      function updateStats(history, current) {
        var metrics = [
          { key: 'temp', label: '🌡️ 温度',  unit: '°C',  dec: 1 },
          { key: 'hum',  label: '💧 湿度',  unit: '%',   dec: 1 },
          { key: 'lux',  label: '☀️ 光照',  unit: 'lx',  dec: 0 },
          { key: 'soil', label: '🌱 土壤',  unit: '%',   dec: 0 },
          { key: 'eco2', label: '☁️ eCO2', unit: 'ppm', dec: 0 },
          { key: 'tvoc', label: '🧪 TVOC', unit: 'ppb', dec: 0 }
        ];
        var rows = '';
        metrics.forEach(function(m) {
          var s = calcStats(history[m.key]);
          var cur = (current && current[m.key] !== undefined) ? parseFloat(current[m.key]).toFixed(m.dec) + ' ' + m.unit : '--';
          if (s) {
            rows += '<tr class="border-b border-slate-100 hover:bg-slate-50">';
            rows += '<td class="px-3 py-2 font-medium text-gray-700 text-left">' + m.label + '</td>';
            rows += '<td class="px-3 py-2 text-blue-600 font-mono">' + s.min.toFixed(m.dec) + ' ' + m.unit + '</td>';
            rows += '<td class="px-3 py-2 text-red-500 font-mono">'  + s.max.toFixed(m.dec) + ' ' + m.unit + '</td>';
            rows += '<td class="px-3 py-2 text-gray-600 font-mono">' + s.avg.toFixed(m.dec) + ' ' + m.unit + '</td>';
            rows += '<td class="px-3 py-2 text-green-700 font-mono font-bold">' + cur + '</td>';
            rows += '</tr>';
          }
        });
        if (rows) document.getElementById('stats-body').innerHTML = rows;
      }

      function updateAlerts(current) {
        var alerts = [];
        if (current.temp !== undefined) {
          if (current.temp > 35)      alerts.push({ cls: 'alert-danger', msg: '⚠️ 温度过高 (' + current.temp.toFixed(1) + '°C)，请检查通风！' });
          else if (current.temp < 10) alerts.push({ cls: 'alert-danger', msg: '🥶 温度过低 (' + current.temp.toFixed(1) + '°C)，请检查加热！' });
          else if (current.temp > 28) alerts.push({ cls: 'alert-warn',   msg: '🌡️ 温度偏高 (' + current.temp.toFixed(1) + '°C)' });
          else if (current.temp < 15) alerts.push({ cls: 'alert-warn',   msg: '🌡️ 温度偏低 (' + current.temp.toFixed(1) + '°C)' });
        }
        if (current.hum !== undefined) {
          if (current.hum > 90)       alerts.push({ cls: 'alert-danger', msg: '💧 湿度过高 (' + current.hum.toFixed(1) + '%)，有霉变风险！' });
          else if (current.hum < 20)  alerts.push({ cls: 'alert-warn',   msg: '💧 湿度偏低 (' + current.hum.toFixed(1) + '%)' });
        }
        if (current.soil !== undefined) {
          if (current.soil < 20)      alerts.push({ cls: 'alert-danger', msg: '🌱 土壤严重缺水 (' + current.soil + '%)，请立即浇水！' });
          else if (current.soil < 30) alerts.push({ cls: 'alert-warn',   msg: '🌱 土壤水分偏低 (' + current.soil + '%)' });
          else if (current.soil > 80) alerts.push({ cls: 'alert-warn',   msg: '🌱 土壤水分偏高 (' + current.soil + '%)' });
        }
        if (current.eco2 !== undefined) {
          if (current.eco2 > 1500)     alerts.push({ cls: 'alert-danger', msg: '☁️ CO2严重超标 (' + current.eco2 + 'ppm)，需紧急通风！' });
          else if (current.eco2 > 1000) alerts.push({ cls: 'alert-warn', msg: '☁️ CO2浓度偏高 (' + current.eco2 + 'ppm)' });
        }
        if (current.tvoc !== undefined) {
          if (current.tvoc > 660)      alerts.push({ cls: 'alert-danger', msg: '🧪 有机挥发物严重超标 (' + current.tvoc + 'ppb)！' });
          else if (current.tvoc > 220) alerts.push({ cls: 'alert-warn',   msg: '🧪 有机挥发物偏高 (' + current.tvoc + 'ppb)' });
        }
        var list = document.getElementById('alert-list');
        if (alerts.length === 0) {
          list.innerHTML = '<div class="text-green-600 font-medium">✅ 环境状态良好，无预警</div>';
        } else {
          list.innerHTML = alerts.map(function(a) { return '<div class="' + a.cls + '">' + a.msg + '</div>'; }).join('');
        }
      }

      // ----- UI 更新函数 -----
      function updateUI(data) {
        if (data.current) {
          document.getElementById('val-temp').innerText = (data.current.temp !== undefined) ? data.current.temp.toFixed(1) + ' °C' : '-- °C';
          document.getElementById('val-hum').innerText  = (data.current.hum  !== undefined) ? data.current.hum.toFixed(1)  + ' %'  : '-- %';
          document.getElementById('val-lux').innerText  = (data.current.lux  !== undefined) ? data.current.lux.toFixed(0)  + ' lx' : '-- lx';
          document.getElementById('val-soil').innerText = (data.current.soil !== undefined) ? data.current.soil + ' %'             : '-- %';
          document.getElementById('val-eco2').innerText = (data.current.eco2 !== undefined) ? data.current.eco2 + ' ppm'           : '-- ppm';
          document.getElementById('val-tvoc').innerText = (data.current.tvoc !== undefined) ? data.current.tvoc + ' ppb'           : '-- ppb';
          setSensorBadge('badge-temp', data.current.temp, 10, 28, 35);
          setSensorBadge('badge-hum',  data.current.hum,  20, 80, 90);
          setSensorBadge('badge-lux',  data.current.lux,  200, 800, 2000);
          setSensorBadge('badge-soil', data.current.soil, 30, 60, 80);
          setSensorBadge('badge-eco2', data.current.eco2, 400, 1000, 1500);
          setSensorBadge('badge-tvoc', data.current.tvoc, 0, 220, 660);
          updateAlerts(data.current);
        }

        if (data.system) {
          if (data.system.heap_total && data.system.heap_free) {
            let heapUsage = ((data.system.heap_total - data.system.heap_free) / data.system.heap_total) * 100;
            document.getElementById('val-heap').innerText = heapUsage.toFixed(1) + ' %';
          }
          if (data.system.rssi) {
            document.getElementById('val-rssi').innerText = data.system.rssi + ' dBm';
            let qualityEl = document.getElementById('val-rssi-quality');
            qualityEl.className = 'font-bold';
            if (data.system.rssi >= -50) { qualityEl.innerText = '极好'; qualityEl.classList.add('text-quality-excellent'); }
            else if (data.system.rssi >= -70) { qualityEl.innerText = '良好'; qualityEl.classList.add('text-quality-good'); }
            else if (data.system.rssi >= -80) { qualityEl.innerText = '一般'; qualityEl.classList.add('text-quality-fair'); }
            else { qualityEl.innerText = '较差'; qualityEl.classList.add('text-quality-poor'); }
          }
          if (data.system.chip_rev)  document.getElementById('val-chip-rev').innerText  = 'Rev ' + data.system.chip_rev;
          if (data.system.mac)       document.getElementById('val-mac').innerText        = data.system.mac;
          if (data.system.ip)        document.getElementById('val-ip').innerText         = data.system.ip;
          if (data.system.cpu_freq)  document.getElementById('val-cpu-freq').innerText   = data.system.cpu_freq + ' MHz';
          if (data.system.cpu_cores) document.getElementById('val-cpu-cores').innerText  = data.system.cpu_cores + ' 核';
          if (data.system.uptime !== undefined) document.getElementById('val-uptime').innerText = formatUptime(data.system.uptime);
          if (data.system.sdk_ver)   document.getElementById('val-sdk').innerText        = data.system.sdk_ver;
        }

        if (data.history && data.history.time && data.history.time.length > 0) {
          chartTH.setOption({ xAxis: { data: data.history.time }, series: [{ data: data.history.temp || [] }, { data: data.history.hum  || [] }] });
          chartLS.setOption({ xAxis: { data: data.history.time }, series: [{ data: data.history.lux  || [] }, { data: data.history.soil || [] }] });
          chartAQ.setOption({ xAxis: { data: data.history.time }, series: [{ data: data.history.eco2 || [] }, { data: data.history.tvoc || [] }] });
          updateStats(data.history, data.current);
        }

        if (data.sched) {
          updateSchedUI(data.sched);
        }

        if (data.state) {
          var isAuto = (data.state.mode === 'auto');
          document.getElementById('cb-mode').checked = isAuto;
          var modeText = document.getElementById('mode-text');
          modeText.innerText = isAuto ? '自动' : '手动';
          modeText.className = isAuto ? 'ml-3 font-bold text-lg w-28 text-left text-green-600' : 'ml-3 font-bold text-lg w-28 text-left text-orange-500';
          ['pump', 'light', 'heater', 'fan'].forEach(function(dev) {
            var cb = document.getElementById('cb-' + dev);
            if (cb) { cb.disabled = isAuto; cb.checked = (data.state[dev] === 1); }
          });
        }
      }

      window.saveSchedule = function() {
        if (socket && socket.readyState === WebSocket.OPEN) {
          socket.send(JSON.stringify({
            action: 'set_sched',
            fan_en: document.getElementById('sched-fan-en').checked,
            fan_start: document.getElementById('sched-fan-start').value || '00:00',
            fan_end: document.getElementById('sched-fan-end').value || '00:00',
            light_en: document.getElementById('sched-light-en').checked,
            light_start: document.getElementById('sched-light-start').value || '00:00',
            light_end: document.getElementById('sched-light-end').value || '00:00'
          }));
        }
      };

      // Update schedule UI from received data
      function updateSchedUI(sched) {
        if (!sched) return;
        document.getElementById('sched-fan-en').checked = sched.fan_en;
        document.getElementById('sched-fan-start').value = sched.fan_start;
        document.getElementById('sched-fan-end').value = sched.fan_end;
        document.getElementById('sched-light-en').checked = sched.light_en;
        document.getElementById('sched-light-start').value = sched.light_start;
        document.getElementById('sched-light-end').value = sched.light_end;
      }

      window.toggleMode = function() {
        var isAuto = document.getElementById('cb-mode').checked;
        if (socket && socket.readyState === WebSocket.OPEN) socket.send(JSON.stringify({ action: 'set_mode', mode: isAuto ? 'auto' : 'manual' }));
      };
      window.toggleDevice = function(dev) {
        var isOn = document.getElementById('cb-' + dev).checked;
        if (socket && socket.readyState === WebSocket.OPEN) socket.send(JSON.stringify({ action: 'set_device', device: dev, state: isOn ? 1 : 0 }));
      };

      // ----- AI 助手交互逻辑 -----
      function appendChat(role, msg) {
        const box = document.getElementById('ai-chat-box');
        const outer = document.createElement('div');
        outer.className = role === 'user' ? 'flex justify-end pr-2 pl-8' : 'flex justify-start pl-2 pr-8';
        const inner = document.createElement('div');
        inner.className = role === 'user' 
          ? 'bg-blue-600 text-white p-3 rounded-2xl rounded-tr-none text-sm break-words' 
          : 'bg-white border border-gray-100 shadow-sm p-3 rounded-2xl rounded-tl-none text-sm text-gray-700 leading-relaxed break-words whitespace-pre-wrap';
        inner.innerText = msg;
        outer.appendChild(inner);
        box.appendChild(outer);
        box.scrollTop = box.scrollHeight;
      }

      window.askAI = function() {
        if (socket && socket.readyState === WebSocket.OPEN) {
          const btn = document.getElementById('btn-ai-analyze');
          btn.disabled = true;
          btn.innerText = '分析中...';
          btn.classList.add('opacity-50', 'cursor-not-allowed');
          appendChat('user', '请根据当前环境数据（温湿度、CO2、光照）分析大棚状态并给出农业建议。');
          socket.send(JSON.stringify({ action: 'ai_analyze' }));
        }
      };

      window.sendAIChat = function() {
        const input = document.getElementById('ai-input');
        const q = input.value.trim();
        if (!q || !socket || socket.readyState !== WebSocket.OPEN) return;
        appendChat('user', q);
        socket.send(JSON.stringify({ action: 'ai_chat', question: q }));
        input.value = '';
      };

      document.getElementById('ai-input') && document.getElementById('ai-input').addEventListener('keypress', function(e) {
        if (e.key === 'Enter') sendAIChat();
      });

      // ----- 扩展 updateUI 处理 AI 响应 -----
      const originalUpdateUI = updateUI;
      updateUI = function(data) {
        if (data.ai_resp) {
          const btn = document.getElementById('btn-ai-analyze');
          btn.disabled = false;
          btn.innerText = '实时环境分析';
          btn.classList.remove('opacity-50', 'cursor-not-allowed');
          appendChat('ai', data.ai_resp);
        }
        originalUpdateUI(data);
      };

      window.addEventListener('resize', function() { chartTH.resize(); chartLS.resize(); chartAQ.resize(); });
    })();
  </script>
</body>
</html>
)rawliteral";