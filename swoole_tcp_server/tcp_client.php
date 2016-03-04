<?php
/**
 * php socket client
 * 封装发送socket的方法
 * 
 * @author luorenbin
 * @version 2015-10
 */
class Socketclient {
    //服务器地址
    private $srv_addr = "";
    //服务器端口
    private $srv_port = "";
    //连接超时时间   
    private $time_out = 10;
    //响应超时时间
    private $rsp_timeout = 10;
    //连接
    private $conn = null;

    /**
     * 构造方法
     */
    public function __construct($trade_code) {
        $this->srv_addr = "127.0.0.1";
        $this->srv_port = "9501";
    }
    
    
    /**
     * [sendRequest description]
     * 
     * @param [type] $header
     *            [description]
     * @param [type] $content
     *            [description]
     * @return [type] [description]
     */
    private function sendRequest($content) {
        $this->connect ();
        
        if ($this->conn) {
            
            $start = microtime(true);
            fwrite ( $this->conn, $content );
            
            $response = "";
            
            //设置超时
            stream_set_blocking($this->conn, TRUE);
            stream_set_timeout($this->conn,$this->rsp_timeout);
            
            $info = stream_get_meta_data($this->conn);
            while ((!feof($this->conn)) && (!$info['timed_out'])) {
                $response .= fgets($this->conn, 2048);
                $info = stream_get_meta_data($this->conn);
            }
            
            //超时记录日志
            
            $this->disconnect ();
            return $response;
        } else {
            
            return false;
        }
    }
    
    
    /**
     * connect to the socket
     * @return [type] [description]
     */
    private function connect() {
        if (empty ( $this->srv_addr ) || empty ( $this->srv_port )) {
            return false;
        }
        if (! $this->conn) {
            $this->conn = fsockopen ( $this->srv_addr, $this->srv_port, $errno, $errstr, $this->time_out );
            //$this->conn = stream_socket_client ( $this->srv_addr.":".$this->srv_port, $errno, $errstr, $this->time_out );
            
        }
    }
    
    /**
     * close socket connect
     * @return [type] [description]
     */
    private function disconnect() {
        if ($this->conn) {
            fclose ( $this->conn );
        }
    }
    
   
    
    
}

