# 驱动配置文件说明

1. 把 devices.json.in 文件名修改为 devices.json 。（如果不修改名字则使用默认驱动配置或者宏配置）
2. 编辑 devices.json 修改输入设备的文件名，该配置文件会随后续打包命令一起部署

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


