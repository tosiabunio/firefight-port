#include "headers.h"

int Tools::initialized=0;
int Tools::colors[Tools::_LAST];
Tool Tools::tool[Tools::_LAST];
Tool Tools::radar_tool[Tools::max_radartools];
Tool Tools::smokeblack_tool[Tools::max_smoketools];
Tool Tools::smokewhite_tool[Tools::max_smoketools];
Text Tools::text;

void Tools::init(char *file)
{
  initialized=1;
  text.load(file);
  int r1=0,g1=0,b1=0,r2=0,g2=0,b2=0;
  for (int i=0;i<_LAST;i++)
  {
    colors[i]=0;
  }

  init_color(colors[NET_FIRST],text.string("netcolor1"));
  init_color(colors[NET_SECOND],text.string("netcolor2"));
  init_color(colors[NET_THIRD],text.string("netcolor3"));
  init_color(colors[NET_FOURTH],text.string("netcolor4"));

  init_tool(tool[NET_FIRST],text.string("nettool1"));
  init_tool(tool[NET_SECOND],text.string("nettool2"));
  init_tool(tool[NET_THIRD],text.string("nettool3"));
  init_tool(tool[NET_FOURTH],text.string("nettool4"));
  init_tool(tool[BLACKWHITE],text.string("blackwhite"));
  init_tool(tool[SHADOW],text.string("shadow"));
  init_tool(tool[LSHADOW],text.string("lshadow"));
  init_tool(tool[VLSHADOW],text.string("vlshadow"));
  init_tool(tool[ALIENSMOKE],text.string("aliensmoke"));
  init_tool(tool[DISPINFO],text.string("dispinfo"));
  init_tool(tool[BACKSPRITE],text.string("backfilter"));
  init_tool(tool[LIGHT],text.string("light"));

  tool[HELP].brightness(Layout::helppanel.mr1);

  Tools::init_light_tool();
  Tools::init_smoke_tool();
}

void Tools::quit(void)
{
  text.free();
  initialized=0;
}

void Tools::init_smoke_tool(void)
{
  DBG_CHECK(initialized);
  int i=0,r=0,g=0,b=0,begin=0,step=0;

  text.area("blacksmoke");
  sscanf(text.string("rgb"),"%d,%d,%d",&r,&g,&b);
  begin=text.value("begin");
  step=text.value("step");
  for (i=0;i<max_smoketools;i++)
  {
    smokeblack_tool[i].glass(Color(r,g,b),begin+((max_smoketools-i-1)*step));
  }
  text.endarea();

  text.area("whitesmoke");
  sscanf(text.string("rgb"),"%d,%d,%d",&r,&g,&b);
  begin=text.value("begin");
  step=text.value("step");
  for (i=0;i<max_smoketools;i++)
  {
    smokewhite_tool[i].glass(Color(r,g,b),begin+((max_smoketools-i-1)*step));
  }
  text.endarea();
}

void Tools::init_light_tool(void)
{
  DBG_CHECK(initialized);
  text.area("expl");
  int expltool=text.value("expltool");
  int expldif=text.value("expldif");
  int explnum=LIGHT16-LIGHT1+1;
  for (int i=0;i<explnum;i++)
  {
    tool[i+LIGHT1].brightness(expltool+i*expldif);
  }
  text.endarea();
}

void Tools::init_radar_tool(void)
{
  DBG_CHECK(initialized);
  char alien[256];
  char target[256];
  int alienr=0,alieng=0,alienb=0,alienp=0,targetr=0,targetg=0,targetb=0,targetp=0;
  text.area("radar");
  sprintf(alien,"%salien",GameManager::volume);
  sprintf(target,"%starget",GameManager::volume);
  sscanf(text.string(alien),"%d,%d,%d,%d",&alienr,&alieng,&alienb,&alienp);
  sscanf(text.string(target),"%d,%d,%d,%d",&targetr,&targetg,&targetb,&targetp);
  double r=alienr;
  double g=alieng;
  double b=alienb;
  double p=alienp;
  double rd=(targetr-r)/(double)max_radartools;
  double gd=(targetg-g)/(double)max_radartools;
  double bd=(targetb-b)/(double)max_radartools;
  double pd=(targetp-p)/(double)max_radartools;
  for(double i=0;i<max_radartools;i+=1)
  {
    radar_tool[(int)i].glass(Color((int)(r+i*rd),(int)(g+i*gd),(int)(b+i*bd)),(int)(p+i*pd));
  }
  text.endarea();
}

void Tools::init_tool(Tool& tool,char *descript)
{
  DBG_CHECK(initialized);
  int r1=0,g1=0,b1=0,r2=0,g2=0,b2=0;
  int result=sscanf(descript,"%d,%d,%d,%d,%d,%d",&r1,&g1,&b1,&r2,&g2,&b2);
  if (result>4)
  {
    tool.glass(Color(r1,g1,b1),Color(r2,g2,b2));
  }
  else if (result==4)
  {
    tool.glass(Color(r1,g1,b1),r2);
  }
  else
  {
    tool.brightness(r1);
  }
}

void Tools::init_color(int& color,char *descript)
{
  DBG_CHECK(initialized);
  int r=0,g=0,b=0;
  sscanf(descript,"%d,%d,%d",&r,&g,&b);
  color=Video::closest(r,g,b);
}




