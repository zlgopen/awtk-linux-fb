# 设备配置文件说明

1. 复制模板文件 devices.json.in 并改名为 devices.json，该配置文件会随后续打包命令一起部署。

   （如果运行时没有 devices.json 文件则使用 main_loop_linux.c 默认设备宏配置）

2. 编辑 devices.json 指定输入设备的文件路径及对应的设备类型。

```json
{
    "/dev/fb0" : {
        "type" : "fb"
    },
	"/dev/dri/card0" : {
        "type" : "drm"
    },
	"/dev/input/event0" : {
        "type" : "ts"
    },
	"/dev/input/event1" : {
        "type" : "input"
    },
	"/dev/input/mouse0" : {
        "type" : "mouse"
    }
}
```


