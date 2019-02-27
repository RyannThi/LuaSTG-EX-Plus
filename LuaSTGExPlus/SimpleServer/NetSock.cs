//======================================
//网络连接

using System;                  //控制台打印
using System.Windows.Controls; //控件
using System.Text;             //编码
using System.Collections;      //容器
using System.Threading;        //多线程
using System.Net;              //网络库
using System.Net.Sockets;      //套接字

namespace LuaSTGServer
{
    //==================
    //log
    public enum LOGLEVEL {
        DEBUG   = 1,
        INFO    = 2,
        WARN    = 3,
        ERROR   = 4,
        INVALID = 5,
    }
    public class LOGGER
    {
        //==================
        private LOGLEVEL GLOBAL_LOGLEVEL;
        private TextBox LogWindow;
        //==================
        public LOGGER()
        {
            GLOBAL_LOGLEVEL = LOGLEVEL.INFO;
        }
        public LOGGER(LOGLEVEL g_lv)
        {
            GLOBAL_LOGLEVEL = g_lv;
        }
        public LOGGER(TextBox logW)
        {
            LogWindow = logW;
            GLOBAL_LOGLEVEL = LOGLEVEL.INFO;
        }
        ~LOGGER()
        {
        }
        //==================
        public void Print(LOGLEVEL lv, string msg)
        {
            if (lv >= GLOBAL_LOGLEVEL && msg.Length>0)
            {
                switch (lv)
                {
                    case LOGLEVEL.DEBUG:
                        msg = "[DEBUG]" + msg;
                        break;
                    case LOGLEVEL.INFO:
                        msg = "[INFO]" + msg;
                        break;
                    case LOGLEVEL.WARN:
                        msg = "[WARN]" + msg;
                        break;
                    case LOGLEVEL.ERROR:
                        msg = "[ERROR]" + msg;
                        break;
                }
                msg = msg + '\n';
                LogWindow.Dispatcher.Invoke(new Action(() =>
                {
                    LogWindow.Text = LogWindow.Text + msg;
                    LogWindow.ScrollToEnd();
                }));
            }
        }
        public void PrintConsole(LOGLEVEL lv, string msg)
        {
            if (lv >= GLOBAL_LOGLEVEL)
            {
                switch (lv)
                {
                    case LOGLEVEL.DEBUG:
                        msg = "[DEBUG]" + msg;
                        break;
                    case LOGLEVEL.INFO:
                        msg = "[INFO]" + msg;
                        break;
                    case LOGLEVEL.WARN:
                        msg = "[WARN]" + msg;
                        break;
                    case LOGLEVEL.ERROR:
                        msg = "[ERROR]" + msg;
                        break;
                }
                Console.WriteLine(msg);
            }
        }
    }

    //==================
    //常量，冒着被人打死的危险用枚举来存常量（x
    public enum GLOBAL {
        NETSOCK_LEVEL              = 4,    //log级别
        NETSOCK_MAX_CLIENT_COUNT   = 16,   //最大连接数
        NETSOCK_RECEIVE_BUFFER_LEN = 2048, //TCP接收缓冲区
    }

    //==================
    //线程安全的字符串缓冲区
    public class StringBuffer {
        //==================
        private Queue buffer_object;
        private ReaderWriterLockSlim rwlock_object;
        //==================
        public StringBuffer() {
            rwlock_object = new ReaderWriterLockSlim();
            buffer_object = new Queue();
            buffer_object.Clear();
        }
        ~StringBuffer() {
            buffer_object.Clear();
        }
        //==================
        public void Push(string data) {
            rwlock_object.EnterWriteLock();
            try
            {
                buffer_object.Enqueue(data);
            }
            finally
            {
                rwlock_object.ExitWriteLock();
            }
        }
        public string Get()
        {
            rwlock_object.EnterWriteLock();
            string data;
            try
            {
                if (buffer_object.Count > 0) {
                    data = (string)buffer_object.Dequeue();
                }
                else {
                    data = "";
                }
            }
            finally
            {
                rwlock_object.ExitWriteLock();
            }
            return data;
        }
        public void Clear() {
            rwlock_object.EnterWriteLock();
            try
            {
                buffer_object.Clear();
            }
            finally
            {
                rwlock_object.ExitWriteLock();
            }
        }
        public bool Empty() {
            rwlock_object.EnterReadLock();
            bool rt = false;
            try
            {
                if (buffer_object.Count <= 0)
                {
                    rt = true;
                }
            }
            finally
            {
                rwlock_object.ExitReadLock();
            }
            return rt;
        }
    }

    //==================
    //TCP连接状态
    public enum TCPSTATE {
        FREE,    //空闲
        INSTALL, //已连接
        CONNECT, //收发线程
        PAUSE,   //连接状态，但是不收发，一般不会用到
        STOP,    //停止
        INVALID, //出错，一般不会用到

        LISTEN,  //正在监听状态
    }

    //==================
    //TCP连接，负责收发
    //可以用于服务器作为accept的TCP连接，或者直接创建用于连接到服务器
    public class TCPObject {
        //==================
        //初始为FREE，连接后变成INSTALL状态
        //开始收发后变成CONNECT状态，出错则进入STOP状态
        //手动Reset后恢复FREE状态
        //==================
        public TCPSTATE STATE;
        private LOGGER logger;
        private ReaderWriterLockSlim rwlock_object;
        private Socket socket_object;
        private Thread send_thread;
        private Thread receive_thread;
        private StringBuffer send_buffer;
        private StringBuffer receive_buffer;
        //==================
        public TCPObject(LOGGER logObj)
        {
            logger = logObj;
            socket_object = null;
            send_thread = null;
            receive_thread = null;
            rwlock_object = new ReaderWriterLockSlim();
            send_buffer = new StringBuffer();
            receive_buffer = new StringBuffer();
            STATE = TCPSTATE.FREE;
        }
        ~TCPObject() {
        }
        //==================
        public void Reset() {
            Stop();
            rwlock_object.EnterWriteLock();
            try
            {
                STATE = TCPSTATE.FREE;
                socket_object = null;
                send_buffer.Clear();
                receive_buffer.Clear();
                logger.Print(LOGLEVEL.INFO, "Reset socket object.");
            }
            finally
            {
                rwlock_object.ExitWriteLock();
            }
        }
        public void DirectSet(Socket s) {
            Reset();
            rwlock_object.EnterWriteLock();
            try
            {
                if (s != null) {
                    socket_object = s;
                    STATE = TCPSTATE.INSTALL;
                    logger.Print(LOGLEVEL.INFO, "Direct set socket object succesfully.");
                }
                else {
                    logger.Print(LOGLEVEL.ERROR, "InValid socket object.");
                }
            }
            finally
            {
                rwlock_object.ExitWriteLock();
            }
        }
        public void ConnectTo(string ip, int port) {
            Reset();
            bool fail = false;
            //connect
            rwlock_object.EnterWriteLock();
            try
            {
                Socket s = new Socket(SocketType.Stream, ProtocolType.Tcp);
                s.Connect(ip, port);
                socket_object = s;
            }
            catch (SocketException e)
            {
                fail = true;
                logger.Print(LOGLEVEL.ERROR, e.Message + " Error code: " + e.ErrorCode + ".");
            }
            finally
            {
                rwlock_object.ExitWriteLock();
            }
            //check
            if (fail) {
                Reset();
            }
            else {
                rwlock_object.EnterWriteLock();
                try
                {
                    logger.Print(LOGLEVEL.INFO, "Socket object connect succesfully.");
                    STATE = TCPSTATE.INSTALL;
                }
                finally
                {
                    rwlock_object.ExitWriteLock();
                }
            }
        }
        public void Send(string data) {
            if (data.Length > 0) {
                send_buffer.Push(data);
            }
        }
        public string Receive()
        {
            if (!receive_buffer.Empty()) {
                return receive_buffer.Get();
            }
            else {
                return "";//返回长度为0的字符串
            }
        }
        //==================
        public void Start() {
            rwlock_object.EnterWriteLock();
            try
            {
                if ((STATE == TCPSTATE.INSTALL) || (STATE == TCPSTATE.PAUSE)) {
                    STATE = TCPSTATE.CONNECT;
                    Thread STR = new Thread(this.SendThread);
                    send_thread = STR;
                    send_thread.Start();
                    Thread RTR = new Thread(this.ReceiveThread);
                    receive_thread = RTR;
                    receive_thread.Start();
                    logger.Print(LOGLEVEL.INFO, "Start send-receive thread succesfully.");
                }
                else {
                    logger.Print(LOGLEVEL.ERROR, "Invalid state to start send-receive thread.");
                }
            }
            finally
            {
                rwlock_object.ExitWriteLock();
            }
        }
        public void Stop() {
            rwlock_object.EnterWriteLock();
            try
            {
                if (STATE == TCPSTATE.CONNECT) {
                    try {
                        socket_object.Shutdown(SocketShutdown.Both);
                    }
                    catch (SocketException e)
                    {
                        logger.Print(LOGLEVEL.ERROR, e.Message + " Error code: " + e.ErrorCode + ".");
                    }
                    finally {
                        socket_object.Close();
                    }
                    STATE = TCPSTATE.STOP;
                    logger.Print(LOGLEVEL.INFO, "Socket object Stop.");
                }
            }
            finally
            {
                rwlock_object.ExitWriteLock();
            }
        }
        private bool CheckConnect() {
            rwlock_object.EnterReadLock();
            bool state = true;
            try
            {
                if (STATE != TCPSTATE.CONNECT)
                {
                    state = false;
                }
            }
            finally
            {
                rwlock_object.ExitReadLock();
            }
            return state;
        }
        private void SendThread() {
            while (true)
            {
                //first check
                if (!CheckConnect()) {
                    logger.Print(LOGLEVEL.ERROR, "InValid state to continue send thread.");
                    break;
                }
                if (!send_buffer.Empty()) {
                    string str = send_buffer.Get();
                    byte[] data = Encoding.UTF8.GetBytes(str);
                    bool failed = false;
                    //send
                    //rwlock_object.EnterWriteLock();
                    try
                    {
                        socket_object.Send(data, 0, data.Length, SocketFlags.None);
                    }
                    catch (SocketException e)
                    {
                        logger.Print(LOGLEVEL.ERROR, e.Message + " Error code: " + e.ErrorCode + ".");
                        failed = true;
                    }
                    finally
                    {
                        //rwlock_object.ExitWriteLock();
                    }
                    //check
                    if (failed) {
                        Stop();
                        break;
                    }
                    else {
                        logger.Print(LOGLEVEL.DEBUG, "Send:" + str + " succesfully.");
                    }
                }
                else {
                    Thread.Sleep(0);
                }
            }
        }
        private void ReceiveThread() {
            byte[] data = new byte[(int)GLOBAL.NETSOCK_RECEIVE_BUFFER_LEN];
            while (true) {
                //first check
                if (!CheckConnect()) {
                    logger.Print(LOGLEVEL.ERROR, "InValid state to continue receive thread.");
                    break;
                }
                //data
                Array.Clear(data, 0, data.Length);
                bool failed = false;
                //receive
                string str_out = "";
                //rwlock_object.EnterWriteLock();
                try {
                    int byteCount = socket_object.Receive(data, 0, Math.Min(data.Length,socket_object.Available), SocketFlags.None);
                    if (byteCount > 0) {
                        byte[] tmpdata = new byte[byteCount];
                        Array.Copy(data, tmpdata, byteCount);
                        string str = Encoding.UTF8.GetString(tmpdata);
                        str_out = str;
                        receive_buffer.Push(str);
                    }
                }
                catch (SocketException e) {
                    logger.Print(LOGLEVEL.ERROR, e.Message + " Error code: " + e.ErrorCode + ".");
                    failed = true;
                }
                finally {
                    //rwlock_object.ExitWriteLock();
                }
                //check
                if (failed) {
                    Stop();
                    break;
                }
                else {
                    if (str_out.Length > 0) {
                        logger.Print(LOGLEVEL.DEBUG, "Receive:" + str_out + " succesfully.");
                    }
                }
                Thread.Sleep(0);
            }
        }
    }

    //==================
    //TCP连接，负责监听并返回Socket对象
    public class TCPListener {
        //==================
        public TCPSTATE STATE;
        private LOGGER logger;
        private ReaderWriterLockSlim rwlock_object;
        private Queue socket_buffer;
        private Socket socket_object;
        private Thread listen_thread;
        //==================
        public TCPListener(LOGGER logObj) {
            logger = logObj;
            rwlock_object = new ReaderWriterLockSlim();
            socket_buffer = new Queue();
            socket_buffer.Clear();
            socket_object = null;
            listen_thread = null;
            STATE = TCPSTATE.FREE;
        }
        ~TCPListener() {
        }
        //==================
        public void Reset() {
            Stop();
            rwlock_object.EnterWriteLock();
            try {
                if (STATE != TCPSTATE.FREE) {
                    socket_buffer.Clear();
                    socket_object = null;
                    STATE = TCPSTATE.FREE;
                    logger.Print(LOGLEVEL.INFO, "Listener reset.");
                }
            }
            finally {
                rwlock_object.ExitWriteLock();
            }
        }
        public void Bind(int port) {
            Reset();
            bool fail = false;
            //connect
            rwlock_object.EnterWriteLock();
            try {
                Socket s = new Socket(SocketType.Stream, ProtocolType.Tcp);
                IPEndPoint ep = new IPEndPoint(IPAddress.Any, port);
                s.Bind(ep);
                socket_object = s;
            }
            catch (SocketException e) {
                fail = true;
                logger.Print(LOGLEVEL.ERROR, e.Message + " Error code: " + e.ErrorCode + ".");
            }
            finally {
                rwlock_object.ExitWriteLock();
            }
            //check
            if (fail) {
                Reset();
            }
            else {
                rwlock_object.EnterWriteLock();
                try {
                    logger.Print(LOGLEVEL.INFO, "Listener bind succesfully.");
                    STATE = TCPSTATE.INSTALL;
                }
                finally {
                    rwlock_object.ExitWriteLock();
                }
            }
        }
        public Socket Accept() {
            rwlock_object.EnterWriteLock();
            try {
                if (socket_buffer.Count > 0)
                {
                    return (Socket)socket_buffer.Dequeue();
                }
                else {
                    return null;
                }
            }
            finally {
                rwlock_object.ExitWriteLock();
            }
        }
        //==================
        public void Start()
        {
            rwlock_object.EnterWriteLock();
            try
            {
                if ((STATE == TCPSTATE.INSTALL) || (STATE == TCPSTATE.PAUSE))
                {
                    bool state = true;
                    try
                    {
                        socket_object.Listen((int)GLOBAL.NETSOCK_MAX_CLIENT_COUNT);
                    }
                    catch (SocketException e)
                    {
                        state = false;
                        logger.Print(LOGLEVEL.ERROR, e.Message + " Error code: " + e.ErrorCode + ".");
                    }
                    if (state) {
                        STATE = TCPSTATE.LISTEN;
                        Thread STR = new Thread(this.ListenThread);
                        listen_thread = STR;
                        listen_thread.Start();
                        logger.Print(LOGLEVEL.INFO, "Start listen thread succesfully.");
                    }
                }
                else
                {
                    logger.Print(LOGLEVEL.ERROR, "Invalid state to start listen thread.");
                }
            }
            finally
            {
                rwlock_object.ExitWriteLock();
            }
        }
        public void Stop() {
            rwlock_object.EnterWriteLock();
            try {
                if (STATE == TCPSTATE.LISTEN) {
                    socket_object.Close();
                    STATE = TCPSTATE.STOP;
                    logger.Print(LOGLEVEL.INFO, "Listener stop listen.");
                }
            }
            finally {
                rwlock_object.ExitWriteLock();
            }
        }
        private bool CheckListen()
        {
            rwlock_object.EnterReadLock();
            bool state = true;
            try
            {
                if (STATE != TCPSTATE.LISTEN)
                {
                    state = false;
                }
            }
            finally
            {
                rwlock_object.ExitReadLock();
            }
            return state;
        }
        private void ListenThread() {
            while (true)
            {
                //first check
                if (!CheckListen())
                {
                    logger.Print(LOGLEVEL.ERROR, "InValid state to continue listen thread.");
                    break;
                }
                //accept
                bool state = true;
                try {
                    Socket client = socket_object.Accept();
                    rwlock_object.EnterWriteLock();
                    try
                    {
                        if (client != null) {
                            socket_buffer.Enqueue(client);
                            logger.Print(LOGLEVEL.DEBUG, "Listener accept a new socket object.");
                        }
                    }
                    finally
                    {
                        rwlock_object.ExitWriteLock();
                    }
                }
                catch (SocketException e) {
                    state = false;
                    logger.Print(LOGLEVEL.ERROR, e.Message + " Error code: " + e.ErrorCode + ".");
                }
                finally {
                }
                //check
                if (!state) {
                    Stop();
                    break;
                }
            }
        }
    }

    //==================
    //服务器状态
    public enum SERVERSTATE {
        FREE,    //空闲状态
        INSTALL, //已经处于准备好的状态
        SERVER,  //服务循环中
        PAUSE,   //暂停服务循环，一般不会用到这个状态
        STOP,    //中止状态
        INVALID, //出错，一般不会用到这个状态
    }

    //==================
    //TCP服务端，负责接收连接和广播
    //负责广播信号
    public class BroadcastServer {
        //==================
        public SERVERSTATE STATE;
        private LOGGER logger;
        private TCPObject[] client_object;
        private TCPListener listener_object;
        private ReaderWriterLockSlim rwlock_object;
        private Thread server_thread;
        private Thread broadcast_thread;
        private StringBuffer send_buffer;
        private StringBuffer receive_buffer;
        //==================
        public BroadcastServer(LOGGER logObj) {
            logger = logObj;
            rwlock_object = new ReaderWriterLockSlim();
            listener_object = null;
            //准备客户端
            int count = (int)GLOBAL.NETSOCK_MAX_CLIENT_COUNT;
            client_object = new TCPObject[count];
            for (int i = 0; i < count; i++) {
                client_object[i] = new TCPObject(logObj);
            }
            //buffer
            send_buffer = new StringBuffer();
            receive_buffer = new StringBuffer();
            STATE = SERVERSTATE.FREE;
        }
        ~BroadcastServer() {
        }
        //==================
        public void Reset() {
            Stop();
            rwlock_object.EnterWriteLock();
            try {
                if (STATE != SERVERSTATE.FREE) {
                    listener_object = null;
                    send_buffer.Clear();
                    receive_buffer.Clear();
                    STATE = SERVERSTATE.FREE;
                    logger.Print(LOGLEVEL.INFO, "Broadcast server reset.");
                }
            }
            finally {
                rwlock_object.ExitWriteLock();
            }
        }
        public bool Init(int port) {
            Reset();
            bool state = true;
            rwlock_object.EnterWriteLock();
            try {
                listener_object = new TCPListener(logger);
                listener_object.Bind(port);
                if (listener_object.STATE == TCPSTATE.INSTALL) {
                    STATE = SERVERSTATE.INSTALL;
                    logger.Print(LOGLEVEL.INFO, "Broadcast server init succesfully.");
                }
                else {
                    state = false;
                }
            }
            finally {
                rwlock_object.ExitWriteLock();
            }
            return state;
        }
        //==================
        public void Start() {
            rwlock_object.EnterWriteLock();
            try {
                if ((STATE == SERVERSTATE.INSTALL) || (STATE == SERVERSTATE.PAUSE))
                {
                    //第一步，开始侦听
                    listener_object.Start();
                    if (listener_object.STATE == TCPSTATE.LISTEN)
                    {
                        //第二步，启动服务器线程
                        STATE = SERVERSTATE.SERVER;
                        Thread STR = new Thread(this.ServerThread);
                        server_thread = STR;
                        server_thread.Start();
                        Thread BTR = new Thread(this.BroadcastThread);
                        broadcast_thread = BTR;
                        broadcast_thread.Start();
                        logger.Print(LOGLEVEL.INFO, "Start server thread succesfully.");
                    }
                    else
                    {
                        logger.Print(LOGLEVEL.ERROR, "Listenr failed to start listen thread.");
                    }
                }
                else
                {
                    logger.Print(LOGLEVEL.ERROR, "Invalid state to start server thread.");
                }
        }
            finally {
                rwlock_object.ExitWriteLock();
            }
        }
        public void Stop() {
            rwlock_object.EnterWriteLock();
            try {
                if (STATE == SERVERSTATE.SERVER) {
                    //第一步，关闭自身的监听
                    listener_object.Stop();
                    //第二步，关闭所有客户端的连接
                    int count = (int)GLOBAL.NETSOCK_MAX_CLIENT_COUNT;
                    for (int i = 0; i < count; i++) {
                        if (client_object[i].STATE == TCPSTATE.CONNECT) {
                            client_object[i].Stop();
                        }
                    }
                    //发出中止信号
                    STATE = SERVERSTATE.STOP;
                    logger.Print(LOGLEVEL.INFO, "Server stop.");
                }
            }
            finally {
                rwlock_object.ExitWriteLock();
            }
        }
        private bool CheckServer() {
            rwlock_object.EnterReadLock();
            bool state = true;
            try
            {
                if (STATE != SERVERSTATE.SERVER)
                {
                    state = false;
                }
            }
            finally
            {
                rwlock_object.ExitReadLock();
            }
            return state;
        }
        private void ServerThread() {
            while (true) {
                //first check
                if (!CheckServer())
                {
                    logger.Print(LOGLEVEL.ERROR, "InValid state to continue server listen thread.");
                    break;
                }
                Socket tmpclient = listener_object.Accept();
                if (tmpclient != null) {
                    rwlock_object.EnterWriteLock();
                    try {
                        int count = (int)GLOBAL.NETSOCK_MAX_CLIENT_COUNT;
                        for (int i = 0; i < count; i++)
                        {
                            //找到空闲的客户端
                            if (client_object[i].STATE == TCPSTATE.FREE)
                            {
                                //加载
                                client_object[i].DirectSet(tmpclient);
                                client_object[i].Start();
                                //发送槽位消息
                                char[] hello = new char[4];
                                hello[0] = 'U';
                                hello[1] = 'S';
                                hello[2] = (char)((int)'1' + i);
                                hello[3] = (char)0;
                                string helloword = new string(hello);
                                client_object[i].Send(helloword);
                                //完成，跳出
                                logger.Print(LOGLEVEL.INFO, "Server accept a new socket object.");
                                break;
                            }
                        }
                    }
                    finally {
                        rwlock_object.ExitWriteLock();
                    }
                }
                Thread.Sleep(0);
            }
        }
        private void BroadcastThread() {
            while (true)
            {
                //first check
                if (!CheckServer())
                {
                    logger.Print(LOGLEVEL.ERROR, "InValid state to continue server broadcast thread.");
                    break;
                }
                //广播
                int count = (int)GLOBAL.NETSOCK_MAX_CLIENT_COUNT;
                for (int i = 0; i < count; i++)
                {
                    if (client_object[i].STATE == TCPSTATE.CONNECT)
                    {
                        //对已连接的客户端，检查是否有消息要广播
                        string data = client_object[i].Receive();
                        while (data.Length > 0)
                        {
                            char[] tmpbuffer = data.ToCharArray();
                            if (tmpbuffer[0]!='K') {
                                logger.Print(LOGLEVEL.INFO, data);
                            }
                            //对每一个客户端进行广播
                            for (int j = 0; j < count; j++)
                            {
                                //跳过自身和不在连接状态的客户端
                                if ((client_object[j].STATE == TCPSTATE.CONNECT) && (j != i)) {
                                    client_object[j].Send(data);
                                }
                            }
                            //继续
                            data = client_object[i].Receive();
                        }
                    }
                    else if (client_object[i].STATE == TCPSTATE.STOP) {
                        //对已经停止的客户端，重置为空闲状态
                        client_object[i].Reset();
                    }
                }
                Thread.Sleep(0);
            }
        }
    }
}
