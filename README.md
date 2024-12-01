# ReadTempAndHumidity

controll device
// echo -n "{\"method\":\"getPilot\"}" | nc -4u -w1 192.168.1.202 38899
// https://gist.github.com/LucasPlacentino/1b34b044e0f859721b45c40ae0876e54
// https://github.com/UselessMnemonic/OpenWiz


{
"mac":"d8a01127d90e",
"devMac":"d8a01127d90e",
"moduleName":"ESP24_SHRGBC_01"
}


{"method":"getPilot","env":"pro","result":{"mac":"d8a01127d90e","rssi":-60,"state":false,"sceneId":5,"speed":10,"dimming":14}}


{\"method\":\"setPilot\",\"params\":{\"state\":false}}
echo -n "{\"method\":\"setPilot\",\"params\":{\"state\":false}}" | nc -4u -w1 192.168.1.202 38899
