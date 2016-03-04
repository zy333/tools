<?php
/**
 * @desc： tcp server 服务业务处理类
 * @date 2015-10 
 * @author luorenbin 
 * 
 * 【非常重要】修改此文件要热重启tcp服务.php！！！！
 *
 */
class Business
{
    private $worker_id;
    
    public function __construct($worker_id){
        $this->worker_id = $worker_id;
    }


    /**
     * receive data
     * @param unknown $serv
     * @param unknown $fd
     * @param unknown $from_id
     * @param unknown $data
     */
    function requestHandle($data)
    {
        echo "onReceive\n";
        echo "work id : ".$this->worker_id ."start!";
    	return "Hello World";
    }

   
}


