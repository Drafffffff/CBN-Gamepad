import json,pathlib
root=pathlib.Path.home()/'Games/cbn-gamepad'
common={'ENABLE_JOYSTICK':'true','USE_LANG':'zh_CN','FULLSCREEN':'no','USE_TILES':'true','TILES':'UltimateCataclysm','SOUND_ENABLED':'true','SIDEBAR_SPACERS':'false','PIXEL_MINIMAP':'true','PIXEL_MINIMAP_HEIGHT':'3','ANIMATIONS':'false','AUTOSAVE':'false','AUTO_PICKUP':'false','SCALING_FACTOR':'1','FONT_BLENDING':'true','GPU_LIGHTCAST':'false'}
small=dict(common,FONT_WIDTH='8',FONT_HEIGHT='12',FONT_SIZE='12',MAP_FONT_WIDTH='8',MAP_FONT_HEIGHT='12',MAP_FONT_SIZE='12',OVERMAP_FONT_WIDTH='8',OVERMAP_FONT_HEIGHT='12',OVERMAP_FONT_SIZE='12',TERMINAL_X='80',TERMINAL_Y='30')
desktop=dict(common,FONT_WIDTH='8',FONT_HEIGHT='16',FONT_SIZE='16',MAP_FONT_WIDTH='8',MAP_FONT_HEIGHT='16',MAP_FONT_SIZE='16',OVERMAP_FONT_WIDTH='8',OVERMAP_FONT_HEIGHT='16',OVERMAP_FONT_SIZE='16',TERMINAL_X='480',TERMINAL_Y='135',SCALING_FACTOR='2',FULLSCREEN='fullscreen')
for name,options in [('smoke/profile',dict(small,RENDERER='software',COMPUTE_ACCELERATION='cpu')),('profile-small',small),('profile',desktop)]:
 p=root/name/'config';p.mkdir(parents=True,exist_ok=True)
 (p/'options.json').write_text(json.dumps([{'name':k,'value':v} for k,v in options.items()],indent=2)+'\n')
 (p/'fonts.json').write_text(json.dumps({k:['data/font/Terminus.ttf','data/font/unifont.ttf'] for k in ['typeface','map_typeface','overmap_typeface']},indent=2)+'\n')

(root/'smoke/profile/config/preload.json').write_text(json.dumps({'compute_acceleration':'cpu'}))
