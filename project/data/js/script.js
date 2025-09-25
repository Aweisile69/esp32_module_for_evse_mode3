// 基础配置
const SERVER_URL = "http://192.168.4.1";
let statusUpdateTimer = null;

// 页面加载完成初始化
document.addEventListener('DOMContentLoaded', function() {
    // 初始化日期选择器
    const now = new Date();
    const nextYear = new Date(now.setFullYear(now.getFullYear() + 1)).toISOString().split('T')[0];
    document.getElementById('cardExpire').value = nextYear;

    // 初始化标签页切换
    initTabs();
    
    // 初始化功能模块
    initConnectionStatus();
    startStatusUpdate();
    loadConfig();
    loadCardList();
    loadAlarmList();
    bindFormEvents();
});

// 初始化标签页切换
function initTabs() {
    const tabBtns = document.querySelectorAll('.tab-btn');
    tabBtns.forEach(btn => {
        btn.addEventListener('click', function() {
            const tabId = this.getAttribute('data-tab');
            
            // 更新按钮状态
            tabBtns.forEach(b => b.classList.remove('active'));
            this.classList.add('active');
            
            // 更新内容显示
            document.querySelectorAll('.tab-content').forEach(content => {
                content.classList.remove('active');
            });
            document.getElementById(tabId).classList.add('active');
        });
    });
}

function initConnectionStatus() {
    const connStatusEl = document.getElementById('connStatus');
    // 创建一个控制器用于终止请求
    const controller = new AbortController();
    // 设置超时时间（3秒，根据实际需求调整）
    const timeoutId = setTimeout(() => {
        controller.abort(); // 超时后终止请求
    }, 3000);

    fetch(`${SERVER_URL}/api/ping`, { signal: controller.signal })
        .then(response => {
            clearTimeout(timeoutId); // 清除超时定时器（请求已响应）
            if (response.ok) {
                connStatusEl.className = "status-indicator status-connected";
                connStatusEl.textContent = "已连接设备";
            } else {
                throw new Error("响应状态异常");
            }
        })
        .catch(error => {
            clearTimeout(timeoutId); // 清除超时定时器（请求失败或超时）
            // 无论是网络错误、超时还是响应异常，都显示未连接
            connStatusEl.className = "status-indicator status-error";
            connStatusEl.textContent = "未连接设备";
            // 可选：打印错误详情（调试用）
            console.log("连接检查失败：", error.message);
        });
}

// 启动状态更新
function startStatusUpdate() {
    updateDeviceStatus();
    statusUpdateTimer = setInterval(updateDeviceStatus, 3000);
}

function updateDeviceStatus() {
    // 发起API请求获取设备状态
    fetch(`${SERVER_URL}/api/status`)
        .then(response => {
            // 检查HTTP响应状态
            if (!response.ok) {
                throw new Error(`HTTP错误：${response.status}`);
            }
            return response.json();
        })
        .then(data => {
            // 格式化浮点型数据的工具函数
            // 处理null/undefined，保留1位小数，0值正常显示
            const formatFloatValue = (value) => {
                // 仅当值存在且为有效数字时才格式化
                if (value === null || value === undefined || isNaN(value)) {
                    return '--';
                }
                // 保留1位小数（可根据需求调整位数）
                return parseFloat(value).toFixed(1);
            };

            // 更新电压显示
            const voltageEl = document.getElementById('voltage');
            voltageEl.textContent = `${formatFloatValue(data.voltage)} V`;

            // 更新电流显示
            const currentEl = document.getElementById('current');
            currentEl.textContent = `${formatFloatValue(data.current)} A`;

            // 更新功率显示
            const powerEl = document.getElementById('power');
            powerEl.textContent = `${formatFloatValue(data.power)} W`;

            // 更新充电状态（数字枚举转文本+样式）
            const chargeEl = document.getElementById('chargeStatus');
            const chargeStateMap = {
                0: "启动中",         // EVSE_REBOOT
                1: "空闲",           // EVSE_IDLE
                2: "等待刷卡",       // EVSE_plugWaitSwipe
                3: "等待插枪",       // EVSE_swipeWaitPlug
                4: "准备就绪",       // EVSE_swipePlugReady
                5: "充电中",         // EVSE_CHARGING
                6: "充电暂停",       // EVSE_CHARGE_PAUSE
                7: "充电停止",       // EVSE_CHARGE_STOP
                8: "充电完成",       // EVSE_CHARGE_DONE
                9: "充电故障"        // EVSE_FAULT
            };
            // 获取状态文本（默认未知状态）
            const chargeStateText = chargeStateMap[data.charge_status] || "未知状态";
            chargeEl.textContent = chargeStateText;
            // 设置状态样式
            switch(data.charge_status) {
                case 9:  // 故障状态
                    chargeEl.className = "param-value text-danger";
                    break;
                case 5:  // 充电中
                    chargeEl.className = "param-value text-info";
                    break;
                default:  // 其他状态
                    chargeEl.className = "param-value text-success";
            }

            // 更新网络状态
            const networkEl = document.getElementById('wifiStatus');
            const networkStateMap = {
                0: "断开",
                1: "已连接"
            };
            const networkStateText = networkStateMap[data.net_status] || "未知网络状态";
            networkEl.textContent = networkStateText;
            // 设置网络状态样式
            networkEl.className = data.net_status === 1
                ? "param-value text-success" 
                : "param-value text-warning";

        })
        .catch(err => {
            console.error("更新设备状态失败：", err);
            // 错误状态下显示默认值
            const elements = [
                { id: 'voltage', unit: 'V' },
                { id: 'current', unit: 'A' },
                { id: 'power', unit: 'W' }
            ];
            elements.forEach(item => {
                document.getElementById(item.id).textContent = `-- ${item.unit}`;
            });
            document.getElementById('chargeStatus').textContent = "数据获取失败";
            document.getElementById('wifiStatus').textContent = "数据获取失败";
        });
}

// 加载配置
function loadConfig() {
    fetch(`${SERVER_URL}/api/config`)
        .then(response => response.json())
        .then(config => {
            document.getElementById('OV_threshold').value = config.ov_threshold || 286.0;
            document.getElementById('UV_threshold').value = config.uv_threshold || 154.0;
            document.getElementById('leakageDC').value = config.leakagedc || 30;
            document.getElementById('leakageAC').value = config.leakageac || 30;
            document.getElementById('maxChargeCurrent').value = config.maxcc || 32;
        })
        .catch(err => console.error("加载配置失败：", err));
}

// 加载授权卡列表
function loadCardList() {
    const cardListEl = document.getElementById('cardList');
    fetch(`${SERVER_URL}/api/cards`)
        .then(response => response.json())
        .then(cards => {
            if (cards.length === 0) {
                cardListEl.innerHTML = '<tr><td colspan="4" class="text-center">暂无授权卡数据</td></tr>';
                return;
            }
            
            let html = '';
            cards.forEach((card, index) => {
                html += `
                    <tr>
                        <td>${index + 1}</td>
                        <td>${card.id}</td>
                        <td>${card.expireDate}</td>
                        <td>
                            <button class="btn btn-danger delete-card" data-id="${card.id}">删除</button>
                        </td>
                    </tr>
                `;
            });
            cardListEl.innerHTML = html;
            bindDeleteCardEvents();
        })
        .catch(err => {
            cardListEl.innerHTML = '<tr><td colspan="4" class="text-center text-danger">加载失败</td></tr>';
        });
}

// 加载告警记录
function loadAlarmList() {
    const alarmListEl = document.getElementById('alarmList');
    fetch(`${SERVER_URL}/api/alarms`)
        .then(response => response.json())
        .then(alarms => {
            if (alarms.length === 0) {
                alarmListEl.innerHTML = '<tr><td colspan="4" class="text-center">暂无告警记录</td></tr>';
                return;
            }
            
            let html = '';
            alarms.reverse().forEach((alarm, index) => {
                const statusText = alarm.handled ? '已处理' : '未处理';
                const statusClass = alarm.handled ? 'text-success' : 'text-warning';
                html += `
                    <tr>
                        <td>${index + 1}</td>
                        <td>${alarm.time}</td>
                        <td>${alarm.coverStatus === 'open' ? '异常打开' : '正常关闭'}</td>
                        <td class="${statusClass}">${statusText}</td>
                    </tr>
                `;
            });
            alarmListEl.innerHTML = html;
        })
        .catch(err => {
            alarmListEl.innerHTML = '<tr><td colspan="4" class="text-center text-danger">加载失败</td></tr>';
        });
}

// 绑定表单事件
function bindFormEvents() {
    // 保存配置
    document.getElementById('configForm').addEventListener('submit', function(e) {
        e.preventDefault();
        const configData = {
            voltageDeviation: parseInt(document.getElementById('voltageDeviation').value),
            maxCurrent: parseInt(document.getElementById('maxCurrent').value),
            cpDeviation: parseInt(document.getElementById('cpDeviation').value),
            leakageAC: parseInt(document.getElementById('leakageAC').value),
            leakageDC: parseInt(document.getElementById('leakageDC').value)
        };

        fetch(`${SERVER_URL}/api/config`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(configData)
        })
        .then(response => response.json())
        .then(res => {
            if (res.success) {
                const msgEl = document.getElementById('configMsg');
                msgEl.textContent = "配置保存成功！";
                msgEl.classList.remove('d-none');
                setTimeout(() => msgEl.classList.add('d-none'), 3000);
            }
        })
        .catch(err => console.error("保存配置失败：", err));
    });

    // 恢复默认配置
    document.getElementById('resetConfigBtn').addEventListener('click', function() {
        if (confirm("确定要恢复默认配置吗？")) {
            document.getElementById('voltageDeviation').value = 5;
            document.getElementById('maxCurrent').value = 32;
            document.getElementById('cpDeviation').value = 10;
            document.getElementById('leakageAC').value = 30;
            document.getElementById('leakageDC').value = 6;
        }
    });

    // 添加授权卡
    document.getElementById('cardAddForm').addEventListener('submit', function(e) {
        e.preventDefault();
        const cardId = document.getElementById('cardId').value;
        const cardExpire = document.getElementById('cardExpire').value;
        const msgEl = document.getElementById('cardAddMsg');

        if (!/^\d{8}$/.test(cardId)) {
            msgEl.textContent = "卡号必须为8位数字！";
            msgEl.className = "mt-2 text-danger";
            msgEl.classList.remove('d-none');
            return;
        }

        fetch(`${SERVER_URL}/api/cards`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id: cardId, expireDate: cardExpire })
        })
        .then(response => response.json())
        .then(res => {
            if (res.success) {
                msgEl.textContent = "授权卡添加成功！";
                msgEl.className = "mt-2 text-success";
                document.getElementById('cardId').value = "";
                setTimeout(loadCardList, 500);
            } else {
                msgEl.textContent = "添加失败：" + res.msg;
                msgEl.className = "mt-2 text-danger";
            }
            msgEl.classList.remove('d-none');
            setTimeout(() => msgEl.classList.add('d-none'), 3000);
        })
        .catch(err => {
            msgEl.textContent = "添加失败，请重试！";
            msgEl.className = "mt-2 text-danger";
            msgEl.classList.remove('d-none');
            setTimeout(() => msgEl.classList.add('d-none'), 3000);
        });
    });

    // 清空告警记录
    document.getElementById('clearAlarmBtn').addEventListener('click', function() {
        if (confirm("确定要清空所有告警记录吗？")) {
            fetch(`${SERVER_URL}/api/alarms`, { method: 'DELETE' })
                .then(response => response.json())
                .then(res => {
                    if (res.success) loadAlarmList();
                });
        }
    });
}

// 绑定删除卡事件
function bindDeleteCardEvents() {
    document.querySelectorAll('.delete-card').forEach(btn => {
        btn.addEventListener('click', function() {
            const cardId = this.getAttribute('data-id');
            if (confirm(`确定要删除授权卡 ${cardId} 吗？`)) {
                fetch(`${SERVER_URL}/api/cards/${cardId}`, { method: 'DELETE' })
                    .then(response => response.json())
                    .then(res => {
                        if (res.success) loadCardList();
                    });
            }
        });
    });
}

// 页面关闭清理
window.addEventListener('beforeunload', function() {
    if (statusUpdateTimer) clearInterval(statusUpdateTimer);
});
