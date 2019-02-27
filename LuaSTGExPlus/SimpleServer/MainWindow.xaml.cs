using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace LuaSTGServer
{
    public partial class MainWindow : Window
    {
        //===================
        private BroadcastServer server;
        private bool start;
        private Button CtrlButton;
        private TextBox PortNumber;
        private TextBox LogWindow;
        private LOGGER logger;
        //===================
        public MainWindow()
        {
            //加载XAML布局
            InitializeComponent();
            _init_help_word();
            //装载按键
            CtrlButton = StartStop;
            PortNumber = PortText;
            PortNumber.MaxLines = 1;//最大行数1行，索引0
            LogWindow = LogText;
            LogText.Clear();
            //加载服务器
            logger = new LOGGER(LogWindow);
            server = new BroadcastServer(logger);
            start = false;
        }
        //===================
        private void StartStop_Click(object sender, RoutedEventArgs e)
        {
            if (start)
            {
                //已经启动服务器
                //停止服务器
                server.Stop();
                //设置控件样式
                PortNumber.IsReadOnly = false;
                CtrlButton.Content = "启动";
                //立flag
                start = false;
            }
            else
            {
                //还未启动服务器
                //读取输入框获取的内容
                string buffer = "" + PortNumber.GetLineText(0);
                //检查输入的是否是数字
                int port;
                if (int.TryParse(buffer, out port))
                {
                    if (port >= 0 && port <= 65535)
                    {
                        //启动服务
                        if (server.Init(port))
                        {
                            //绑定端口成功，启动
                            server.Start();
                            //设置控件样式
                            PortNumber.IsReadOnly = true;
                            CtrlButton.Content = "停止";
                            //立flag
                            start = true;
                        }
                        else
                        {
                            MessageBox.Show("无法启动服务", "LuaSTGServer Error", MessageBoxButton.OK, MessageBoxImage.Error);
                        }
                    }
                    else
                    {
                        MessageBox.Show("端口号必须在 0 到 65535 之间", "LuaSTGServer Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
                else
                {
                    MessageBox.Show("端口号必须是数字", "LuaSTGServer Error", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
        }
        private void ClearLog_Click(object sender, RoutedEventArgs e)
        {
            LogText.Dispatcher.Invoke(new Action(() =>
            {
                LogText.Clear();
            }));
        }
        private void Help_Click(object sender, RoutedEventArgs e)
        {
            MessageBox.Show(_help_word, "About LuaSTGServer", MessageBoxButton.OK, MessageBoxImage.Information);
        }
        private void MainWindow_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            //擦屁股
            server.Stop();
            start = false;
        }
        //===================
        private string _help_word;
        private void _init_help_word() {
            string help = "";
            help += "    LuaSTGServer基于LuaSTG Ex Plus的功能编写，与原有的LuaSTGServer功能完全一样。不过";
            help += "与原有的控制台界面的C++版LuaSTGServer不同，基于C#重写的LuaSTGServer新增了用户界面，";
            help += "并且日志功能被强化。不足的地方就是，它需要.Net运行库，而且是由C#编写的，性能上肯定";
            help += "比不上原来的服务端。本人刚学两天C#，写的东西不是很好，请见谅。\n";
            help += "\n";
            help += "    LuaSTGServer主要用于LuaSTG Ex Plus的联机功能，作为广播服务器使用。";
            help += "主机端打开服务器后，输入端口号启动服务，打开游戏后，连接到127.0.0.1 + 你输入的端口号。";
            help += "其他玩家需要连接到主机的IP地址和服务器端口号，推荐局域网联机，否则需要端口映射或者搭";
            help += "建虚拟局域网。异地游戏一般会使用游侠联机。\n";
            help += "\n";
            help += "    STG联机时网络延迟一般不能超过64ms，否则游戏体验会严重下降。\n";
            help += "\n";
            help += "    C#版LuaSTGServer，即本程序由Xiliusha编写，原C++版LuaSTGServer由ESC编写。\n";
            _help_word = help;
        }
    }
}
