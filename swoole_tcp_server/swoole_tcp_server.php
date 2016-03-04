<?php
/**
 * @desc： tcp server 服务
 * @date 2015-10 
 * @author luorenbin 
 *
 * 用途：使用php搭建一个tcp服务
 * 依赖：php swoole扩展
 * 环境：使用前必须确保安装了swoole扩展，可以使用“php -m | grep swoole” 查看
 * 使用：不要直接执行该脚本，使用方法为配置swoole_tcp_server_monitor.sh到cron，每分钟执行一次
 * 消息转发流程： 
 *  1、tcpserver只负责消息的转发业务
 *  2、发送给api格式为post,参数为msg=原文
 *  流程图：
 *      存管系统------>送消息------->tcpserver------->转发--------->API
 *         ^                       |    ^                         |    
 *         |                       |    |                         |
 *         <---------返回结果-------<     <---------返回结果---------<
 *  
 */

class Server
{
    protected $serv;
    protected $bus;

    /**
     * server run
     */
    function run()
    {
        $serv = new swoole_server('0.0.0.0', 9501, SWOOLE_BASE);
        $serv->set(array(
                'worker_num' => 3 ,   //工作进程数量
                'daemonize' => true, //是否作为守护进程
        ));

        $this->serv  = $serv;
        $serv->on('receive', array($this, 'onReceive'));
        $serv->on('close', array($this, 'onClose'));
        $serv->on('start', array($this, 'onStart'));
        $serv->on('WorkerStart', array($this, 'onWorkerStart'));
        $serv->start();
    }

    function onStart( $serv ) {
        echo "Start\n";
    }
    
    function onWorkerStart($serv , $worker_id){
        echo "onWorkerStart\n";
        //热重启，修改代码后再此处重新加载，使用命令sudo  kill -USR1  master_process_id
        //请写文件的绝对路径
        require_once "swoole_tcp_server_business.php";
        $this->bus =  new Business($worker_id);
    }
    /**
     * receive data
     * @param unknown $serv
     * @param unknown $fd
     * @param unknown $from_id
     * @param unknown $data
     */
    function onReceive($serv, $fd, $from_id, $data)
    {
        echo "onReceive\n";
        $response = "";
        try {
            $response = $this->bus->requestHandle($data);
        } catch (Exception $e) {
            
        }
        
        $serv->send($fd,$response);
        $this->serv->close($fd);
    }

    /**
     * close client connect
     * @param unknown $serv
     * @param unknown $fd
     */
    function onClose($serv, $fd){
        echo "onClose\n";
    }
        
}

$server= new Server;
$server->run();



