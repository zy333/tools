#/bin/bash
BASE_PATH=`pwd`
MODULE_SO_NAME="ngx_http_ezubo_common_module.so"
if [ -d "${BASE_PATH}/temp" ];then
    rm -rf "${BASE_PATH}/temp"
fi
mkdir "${BASE_PATH}/temp"

if [ -d "${BASE_PATH}/output" ];then
    rm -rf "${BASE_PATH}/output"
fi
mkdir "${BASE_PATH}/output"

cd "${BASE_PATH}/temp"
cp "${BASE_PATH}/../tengine-2.1.0.tar.tz" ./
tar zxvf tengine-2.1.0.tar.tz
${BASE_PATH}/temp/tengine-2.1.0/sbin/dso_tool --add-module=${BASE_PATH} --dst=${BASE_PATH}/output
if [ ! -f "${BASE_PATH}/output/${MODULE_SO_NAME}" ];then
    echo "build fail!"
else
    echo "build succ!"
fi
