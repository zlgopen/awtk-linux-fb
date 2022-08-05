# 如何支持鼠标滚轮事件

由于不同的嵌入式 linux 硬件其鼠标驱动文件对滚轮事件的表现不同，甚至部分硬件的驱动并不支持滚轮事件，因此，鼠标滚轮事件并不作为通用功能点直接加到 awtk-linux-fb 的源码里面。

此处，以树莓派为例，讲解如何让 awtk-linux-fb 支持鼠标滚轮事件，其他嵌入式 Linux 平台操作类似。

## 1.判断鼠标驱动是否支持滚轮事件

在终端中执行以下命令查看输入驱动的具体属性：

```bash
cat /proc/bus/input/devices
```

例如，树莓派中鼠标驱动的属性如下，其中有 "B: REL = xxx" 的字样，即表示该鼠标驱动支持相对坐标事件(EV\_REL)，其中包括鼠标滚轮事件：

> 相对坐标事件 (REL) 的类型通常有 REL_X、REL_Y、REL_WHEEL，分别表示鼠标在 x、y 方向上移动和鼠标滚轮滚动。

```bash
I: Bus=0003 Vendor=093a Product=2510 Version=0111
N: Name="PixArt USB Optical Mouse"
P: Phys=usb-3f980000.usb-1.1.3/input0
S: Sysfs=/devices/platform/soc/3f980000.usb/usb1/1-1/1-1.1/1-1.1.3/1-1.1.3:1.0/0003:093A:2510.0004/input/input4
U: Uniq=
H: Handlers=mouse0 event3
B: PROP=0
B: EV=17
B: KEY=70000 0 0 0 0 0 0 0 0
B: REL=903
B: MSC=10
```

## 2.找到正确的鼠标设备文件名

根据上述鼠标驱动文件属性，可知鼠标设备文件有两个，分别是 /dev/input/mouse0 和 /dev/input/event3，通过 "hexdump /dev/input/xxx" 命令找到能正确识别滚轮事件的鼠标设备文件名。

例如，在终端中执行以下命令：

```bash
hexdump "/dev/input/event3"
```

然后，滚动鼠标滚轮，有输出相关数据，并且向上滚动与向下滚动时，其输出信息不同，即表明该鼠标设备文件能正确识别滚轮事件。

> 也可通过命令 "cat /dev/input/event3 | od -t x1 -w16" 来查看指定格式的输出，od 命令的使用方法请上网搜索。

## 3.适配鼠标滚轮事件

awtk-linux-fb 在树莓派中，鼠标消息处理线程是 awtk-port/input_thread/mouse_thread.c 文件中的 input_dispatch_one_event() 函数，因此只需在该函数中捕捉鼠标驱动文件的滚轮事件，转化成 AWTK 的 EVT\_WHEEL 事件，再调用 input_dispatch() 函数将其分发给 AWTK 即可。

### 3.1 捕捉滚轮事件并将其转化后分发给AWTK

鼠标设备文件返回的滚轮事件 type 为 EV_REL，其中 code 对应 REL_WHEEL（垂直滚动）和 REL_HWHEEL（水平滚轮），事件的 value 对应前后左右的方向和滚动的次数（单位）。

> Linux 驱动事件详情请参阅：https://www.kernel.org/doc/Documentation/input/event-codes.txt。

此处仅以 REL_WHEEL 垂直滚动为例，向上滚动时，value 为正数；向下滚动时，value 为负数。滚动一次，value的绝对值即为 1 ，滚动两次，绝对值即为 2。

假设每滚动一次，滚动的距离为 #define MIN_WHEEL_DELTA 12，那么捕捉滚轮事件，并将其转化为 EVT_WHEEL 事件分发给AWTK的的代码如下： 

```c
// awtk-port/input_thread/mouse_thread.c
...
/* 滚动一次的最小距离 */
#define MIN_WHEEL_DELTA 12

/* 设置鼠标滚轮事件 */
static ret_t input_dispatch_set_mouse_wheel_event(run_info_t* info, event_queue_req_t* req, int32_t dy) {
  if (dy > 0) {
    req->wheel_event.dy = tk_max(MIN_WHEEL_DELTA, dy);
  } else if (dy < 0) {
    req->wheel_event.dy = tk_min(-MIN_WHEEL_DELTA, dy);
  }
  req->event.type = EVT_WHEEL;
  return RET_OK;
}

static ret_t input_dispatch_one_event(run_info_t* info) {
  ...
  else if (ret == sizeof(info->data.e)) {
    switch (info->data.e.type) {
      ...
      case EV_REL: {
        switch (info->data.e.code) {
          ...
          /* 从鼠标设备文件中捕捉鼠标滚轮事件 */
          case REL_WHEEL: {
            int32_t dy = MIN_WHEEL_DELTA * info->data.e.value;
            /* 将该事件转化为 AWTK 的 EVT_WHEEL 事件 */
            input_dispatch_set_mouse_wheel_event(info, req, dy);
            break;
          }
          ...
        }

        if (req->event.type == EVT_NONE) {
          req->event.type = EVT_POINTER_MOVE;
        }

        break;
      }
      case EV_SYN: {
        switch (req->event.type) {
          case EVT_KEY_UP:
          case EVT_KEY_DOWN:
          case EVT_CONTEXT_MENU:
          case EVT_POINTER_DOWN:
          case EVT_POINTER_MOVE:
          case EVT_POINTER_UP:
          case EVT_WHEEL: {    /* 将转化后的滚轮事件(EVT_WHEEL)分发给AWTK */
            return input_dispatch(info);
      ...
    }
  }

  return RET_OK;
}
...
```

###　3.2 适配其他嵌入式 Linux 平台或其他事件

其他嵌入式 Linux 平台操作支持鼠标滚轮事件以及其他事件的步骤一样：
1.　从设备文件中捕捉目标事件；
2.　将目标事件转化为AWTK的事件；
3.　分发给AWTK进行处理。

> 需要注意的是若新增其他鼠标事件，还需修改 awtk-prot/main_loop_linux.c 中的 input_dispatch_to_main_loop() 函数，根据消息类型设置 event_queue_req_t 中的 event.size 属性，代码如下：

```c
// awtk-port/main_loop_linux.c
...
ret_t input_dispatch_to_main_loop(void* ctx, const event_queue_req_t* evt, const char* msg) {
  main_loop_t* l = (main_loop_t*)ctx;
  event_queue_req_t event = *evt;
  event_queue_req_t* e = &event;

  if (l != NULL && l->queue_event != NULL) {
    switch (e->event.type) {
      case EVT_KEY_DOWN:
      case EVT_KEY_UP:
      case EVT_KEY_LONG_PRESS: {
        e->event.size = sizeof(e->key_event);
        break;
      }
      case EVT_CONTEXT_MENU:
      case EVT_POINTER_DOWN:
      case EVT_POINTER_MOVE:
      case EVT_POINTER_UP: {
        e->event.size = sizeof(e->pointer_event);
        break;
      }
      /* 设置的滚轮消息 event.size 属性 */
      case EVT_WHEEL: {
        e->event.size = sizeof(e->wheel_event);
        break;
      }
      default:
        break;
    }

    main_loop_queue_event(l, e);
    input_dispatch_print(ctx, e, msg);
  } else {
    return RET_BAD_PARAMS;
  }
  return RET_OK;
}
...
```
