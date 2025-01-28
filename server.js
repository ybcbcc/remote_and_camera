const WebSocket = require('ws');
const http = require('http');
const url = require('url');

// 创建 HTTP 服务器
const server = http.createServer();

// 创建 WebSocket 服务器，将其附加到 HTTP 服务器
const wss = new WebSocket.Server({ noServer: true });

// 存储不同类型的连接
const connections = {
    camera: new Set(),
    remote: new Set(),
};

// 处理升级请求
server.on('upgrade', function upgrade(request, socket, head) {
    const pathname = url.parse(request.url).pathname;

    // 根据路径处理不同的连接
    if (pathname === '/camera' || pathname === '/remote') {
        wss.handleUpgrade(request, socket, head, function done(ws) {
            // 移除 pathname 开头的 '/'
            const type = pathname.slice(1);
            handleConnection(ws, type);
        });
    } else {
        socket.destroy();
    }
});

function handleConnection(ws, type) {
    console.log(`New ${type} connection established`);
    
    // 将连接添加到对应的集合中
    connections[type].add(ws);

    // 设置连接关闭的处理
    ws.on('close', () => {
        console.log(`${type} connection closed`);
        connections[type].delete(ws);
    });

    // 处理消息
    ws.on('message', (message) => {
        console.log(`Received ${type} message:`, message.toString());

        // 如果是摄像头数据，广播给所有查看摄像头的客户端
        if (type === 'camera') {
            connections.camera.forEach((client) => {
                if (client !== ws && client.readyState === WebSocket.OPEN) {
                    client.send(message);
                }
            });
        }
        // 如果是遥控命令，广播给所有遥控器设备
        else if (type === 'remote') {
            connections.remote.forEach((client) => {
                if (client !== ws && client.readyState === WebSocket.OPEN) {
                    client.send(message);
                }
            });
        }
    });

    // 发送连接成功消息
    ws.send(`Connected to ${type} endpoint`);
}

// 错误处理
wss.on('error', (error) => {
    console.error('WebSocket Server Error:', error);
});

// 启动服务器
const PORT = 8080;
server.listen(PORT, () => {
    console.log(`Server is running on port ${PORT}`);
});

// 定期清理断开的连接
setInterval(() => {
    for (const type in connections) {
        connections[type].forEach((ws) => {
            if (ws.readyState === WebSocket.CLOSED) {
                connections[type].delete(ws);
            }
        });
    }
}, 30000);

// 优雅关闭
process.on('SIGTERM', () => {
    console.log('SIGTERM received. Closing WebSocket server...');
    wss.close(() => {
        console.log('WebSocket server closed.');
        process.exit(0);
    });
});
