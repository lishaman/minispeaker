hdmi代码说明
=============

目录结构
---------
jzhdmi
├── api: 由于HDMI的寄存器很多，在api中封装了接口方便上层调用，目前主要是由hdmi_demo_tx调用
├── bsp: 由于他们的驱动是与操作系统无关的，使用纯C书写,　bsp目录下放置了和平台相关的，主要有寄存器访问接口，中断enable/disable接口
├── core: 核心代码，大部分寄存器配置都在此目录下，每个模块对应一个halxxx.c，这个目录下还有audio.c, video.c, packets.c controls.c，这几个文件在halxxx.c的基础上进行了进一步的封装，实现了api.h中的部分接口
├── docs: 和HDMI相关的文档，标准等
├── edid: edid的读取，解析，halxxx.c是封装了寄存器的访问，dtd.c是对CEA-861-D中各种format的描述符的封装，最重要的方法是dtd_Fill, edid.c主要功能是解析EDID, hdmivsdb.c，　shortAudioDesc.c, shortVideo.c封装了相关结构，edid解析的结果就是这些结构
├── hdcp: 和加密相关的，目前我们还没用到
├── hdmi_demo_tx: 一个demo，后面详述
├── phy: 和phy相关的操作，初始化phy, 写phy的寄存器
└── util: 和log待相关的一些小工具
----main.c: 和linux的接口,module_init, module_exit，里面和ioctl相关的代码是没用的，不用去管

git分支说明
------------
control_ok_orig: 基于第一次controller出数时的版本，后来又增加了phy相关的
controller_may_ok: 也是基于第一次controller出数时的版本

重要点说明
-----------
* core/video.c中video_ForceOutput方法中，去掉force = 1的注释，可以强制输出某种颜色或声音，具体的颜色设置在halFrameComposerDebug_ForceVideo中，目前是红色
	1. video force: 调用video_ForceOutput(TRUE), 最终是调用halFrameComposerDebug_ForceVideo完成设置的，只能输出单色
	2. audio force: 8个通道可分别设置（声音会不会很难听*o*), 每个通道可以设置24bit的数据
			要使用audio force需要修改video_ForceOutput的实现，将对halFrameComposerDebug_ForceAudio的调用第二个参数传force
			还需要修改halFrameComposerDebug_ForceAudio的实现，目前没有给８个声道放数据，全０就是静音了
			
	VideoSampler: sampler的参数设置需要注意，hsync, vsync, de的polarity, LCD送过来的是RGB格式
* edid/dtd.c中的dtd_Fill，目前我们用的是format 3，修改dtd->mHSyncPolarity = dtd->mVSyncPolarity可以设置hsync, vsync的极性
* Data Enable的极性是调用api_Initialize时传参设置的，因此如果要改变Data Enable，需要修改api_Initialize或是修改调用它的地方
* HDMI控制器可以自已产生Data Enable，可以在core/video.c的video_VideoSampler方法中调用的halVideoSampler_InternalDataEnableGenerator的最后一个参数设置，1启用HDMI内部generator，0使用我们LCD控制器输入的DE　
* 所有和phy小板相关的都在phy/phy.c中

hdmi_demo说明
-------------
auto流程分析
============
mainFunction
	access_Initialize: 初始化了基地址以及用于寄存器互斥访问的mutex
	system_ThreadCreate: 启动一个线程，等待用户输入并进行分析(tokenize)，存入buffer数组，线程执行体是cliFunction
	如果选择的是auto，调用compliance_start()
		注册hotplug　handler
		api_Initialize()
		compliance_Init()
		然后就进入一个死循环，
			另一个死循环，等待插入或者EDID读取完毕
			compiance_Configure
				目前我们强制使用ecaCode 3，即720x480p
			然后又进入另一个死循环，拔出或是EDID读写出现问题会使这个循环退出
				第一次执行循环时会打印视频格式信息，平时如果我们想打寄存器，也可以加在这
			compiance_Standby
			compiance_Init
			从上面可以看出，compilance测试由三步完成：Init, Configure, Standby
		下面分析相关调用都做了什么，要注意，所有实现都在内核态，用户态只是通过ioctl调用
		api_Initialize
			请参见源代码中添加的中文注释
		compliance_Init(): 准备各种params
			准备product params(product params是什么东东?)
			准备audio params(audio params是什么东东?)
			准备video params(video params是什么东东?): 目前设置的是输入RGB, 输出RGB
		compliance_Configure():
			这里面会遍历sink返回的video descriptor，调用api_Configure设置寄存器，如果设置成功，compiance_Configure
				从代码来看，Audio可以关掉(audioOn), HDCP也可以关掉(hcdpOn)
				api_Configure
					video_Configure
					audio_Configure
					control_Configure
					phy_Configure
					禁掉video_ForceOutput --->ForceOutput会输出蓝屏
				api_Configure出错的原因
					HPD为false,　即线没有插入
