// 基础配置（ESP32 HTTP服务器地址，AP模式下默认IP）
const SERVER_URL = "http://192.168.4.1"; // ESP32 AP模式默认IP，可根据实际配置修改
let statusUpdateTimer = null; // 状态更新定时器

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', function() {
    // 初始化日期选择器（卡授权有效期默认当前日期+1年）
    const now = new Date();
    const nextYear = new Date(now.setFullYear(now.getFullYear() + 1)).toISOString().split('T')[0];
    document.getElementById('cardExpire').value = nextYear;

    // 初始化各模块
    initConnectionStatus(); // 检测连接状态
    startStatusUpdate();    // 启动状态实时更新
    loadConfig();           // 加载当前配置
    loadCardList();         // 加载授权卡列表
    loadAlarmList();        // 加载告警记录

    // 绑定表单提交事件
    bindFormEvents();
});

/**
 * 1. 初始化连接状态
 */
function initConnectionStatus() {
    const connStatusEl = document.getElementById('connStatus');
    fetch(`${SERVER_URL}/api/ping`)
        .then(response => {
            if (response.ok) {
                connStatusEl.className = "alert alert-success d-inline-block px-3 py-1";
                connStatusEl.textContent = "已连接设备";
            } else {
                throw new Error("连接失败");
            }
        })
        .catch(() => {
            connStatusEl.className = "alert alert-danger d-inline-block px-3 py-1";
            connStatusEl.textContent = "未连接设备（请检查WIFI AP）";
        });
}

/**
 * 2. 启动实时状态更新（每3秒刷新一次）
 */
function startStatusUpdate() {
    // 先执行一次
    updateDeviceStatus();
    // 设置定时器
    statusUpdateTimer = setInterval(updateDeviceStatus, 3000);
}

/**
 * 更新设备实时状态
 */
function updateDeviceStatus() {
    fetch(`${SERVER_URL}/api/status`)
        .then(response => response.json())
        .then(data => {
            // 更新基础参数
            document.getElementById('voltage').textContent = `${data.voltage || '--'} V`;
            document.getElementById('current').textContent = `${data.current || '--'} A`;
            document.getElementById('cpSignal').textContent = `${data.cpSignal || '--'} V`;
            
            // 更新漏电保护状态
            const leakageEl = document.getElementById('leakageStatus');
            if (data.leakageStatus === "normal") {
                leakageEl.textContent = "正常";
                leakageEl.className = "param-value text-success";
            } else if (data.leakageStatus === "alarm") {
                leakageEl.textContent = "告警（已断电）";
                leakageEl.className = "param-value text-danger";
            } else {
                leakageEl.textContent = "未检测";
                leakageEl.className = "param-value text-warning";
            }

            // 更新面盖状态
            const coverEl = document.getElementById('coverStatus');
            if (data.coverStatus === "closed") {
                coverEl.textContent = "关闭";
                coverEl.className = "param-value text-success";
            } else if (data.coverStatus === "open") {
                coverEl.textContent = "打开（已告警）";
                coverEl.className = "param-value text-danger";
            }
        })
        .catch(err => {
            console.error("更新状态失败：", err);
            document.getElementById('connStatus').textContent = "连接中断";
        });
}

/**
 * 3. 加载当前配置参数
 */
function loadConfig() {
    fetch(`${SERVER_URL}/api/config`)
        .then(response => response.json())
        .then(config => {
            // 填充表单
            document.getElementById('voltageDeviation').value = config.voltageDeviation || 5;
            document.getElementById('maxCurrent').value = config.maxCurrent || 32;
            document.getElementById('cpDeviation').value = config.cpDeviation || 10;
            document.getElementById('leakageAC').value = config.leakageAC || 30;
            document.getElementById('leakageDC').value = config.leakageDC || 6;
        })
        .catch(err => console.error("加载配置失败：", err));
}

/**
 * 4. 加载授权卡列表
 */
function loadCardList() {
    const cardListEl = document.getElementById('cardList');
    fetch(`${SERVER_URL}/api/cards`)
        .then(response => response.json())
        .then(cards => {
            if (cards.length === 0) {
                cardListEl.innerHTML = '<tr><td colspan="4" class="text-center text-muted">暂无授权卡数据</td></tr>';
                return;
            }
            // 生成表格内容
            let html = '';
            cards.forEach((card, index) => {
                html += `
                    <tr>
                        <td>${index + 1}</td>
                        <td>${card.id}</td>
                        <td>${card.expireDate}</td>
                        <td>
                            <button class="btn btn-sm btn-danger delete-card" data-id="${card.id}">删除</button>
                        </td>
                    </tr>
                `;
            });
            cardListEl.innerHTML = html;
            // 绑定删除按钮事件
            bindDeleteCardEvents();
        })
        .catch(err => {
            console.error("加载授权卡失败：", err);
            cardListEl.innerHTML = '<tr><td colspan="4" class="text-center text-danger">加载失败，请重试</td></tr>';
        });
}

/**
 * 5. 加载告警记录
 */
function loadAlarmList() {
    const alarmListEl = document.getElementById('alarmList');
    fetch(`${SERVER_URL}/api/alarms`)
        .then(response => response.json())
        .then(alarms => {
            if (alarms.length === 0) {
                alarmListEl.innerHTML = '<tr><td colspan="4" class="text-center text-muted">暂无告警记录</td></tr>';
                return;
            }
            // 生成表格内容（按时间倒序）
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
            console.error("加载告警记录失败：", err);
            alarmListEl.innerHTML = '<tr><td colspan="4" class="text-center text-danger">加载失败，请重试</td></tr>';
        });
}

/**
 * 绑定所有表单事件
 */
function bindFormEvents() {
    // 配置表单提交（保存参数）
    document.getElementById('configForm').addEventListener('submit', function(e) {
        e.preventDefault();
        const configData = {
            voltageDeviation: parseInt(document.getElementById('voltageDeviation').value),
            maxCurrent: parseInt(document.getElementById('maxCurrent').value),
            cpDeviation: parseInt(document.getElementById('cpDeviation').value),
            leakageAC: parseInt(document.getElementById('leakageAC').value),
            leakageDC: parseInt(document.getElementById('leakageDC').value)
        };
        // 提交配置到ESP32
        fetch(`${SERVER_URL}/api/config`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(configData)
        })
        .then(response => response.json())
        .then(res => {
            if (res.success) {
                const msgEl = document.getElementById('configMsg');
                msgEl.className = "mt-2 text-success";
                msgEl.textContent = "配置保存成功！";
                msgEl.classList.remove('d-none');
                // 3秒后隐藏提示
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

    // 卡授权添加表单提交
    document.getElementById('cardAddForm').addEventListener('submit', function(e) {
        e.preventDefault();
        const cardId = document.getElementById('cardId').value;
        const cardExpire = document.getElementById('cardExpire').value;
        const msgEl = document.getElementById('cardAddMsg');

        // 表单验证
        if (!/^\d{8}$/.test(cardId)) {
            msgEl.className = "mt-2 text-danger";
            msgEl.textContent = "卡号必须为8位数字！";
            msgEl.classList.remove('d-none');
            return;
        }

        // 提交添加请求
        fetch(`${SERVER_URL}/api/cards`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ id: cardId, expireDate: cardExpire })
        })
        .then(response => response.json())
        .then(res => {
            if (res.success) {
                msgEl.className = "mt-2 text-success";
                msgEl.textContent = "授权卡添加成功！";
                // 清空表单
                document.getElementById('cardId').value = "";
                // 重新加载列表
                setTimeout(loadCardList, 500);
            } else {
                msgEl.className = "mt-2 text-danger";
                msgEl.textContent = "添加失败：" + res.msg;
            }
            msgEl.classList.remove('d-none');
            setTimeout(() => msgEl.classList.add('d-none'), 3000);
        })
        .catch(err => {
            console.error("添加授权卡失败：", err);
            msgEl.className = "mt-2 text-danger";
            msgEl.textContent = "添加失败，请重试！";
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
                })
                .catch(err => console.error("清空告警记录失败：", err));
        }
    });
}

/**
 * 绑定授权卡删除事件
 */
function bindDeleteCardEvents() {
    document.querySelectorAll('.delete-card').forEach(btn => {
        btn.addEventListener('click', function() {
            const cardId = this.getAttribute('data-id');
            if (confirm(`确定要删除授权卡 ${cardId} 吗？`)) {
                fetch(`${SERVER_URL}/api/cards/${cardId}`, { method: 'DELETE' })
                    .then(response => response.json())
                    .then(res => {
                        if (res.success) loadCardList();
                    })
                    .catch(err => console.error("删除授权卡失败：", err));
            }
        });
    });
}

/**
 * 页面关闭时清除定时器
 */
window.addEventListener('beforeunload', function() {
    if (statusUpdateTimer) clearInterval(statusUpdateTimer);
});