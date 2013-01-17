//csv格式通讯录转vcard旧版格式(供诺基亚,outlook)
//csv文件名 MERGE.csv ,无标题,每条一行,格式 "姓名,号码"
//vcard文件在./merged/*.vcf ,每条一个文件

#include<cstdio>
#include<cstring>
#include<string>
using namespace std;

char buf[1024];
const char *vcf_fmt = 
"BEGIN:VCARD\n\
VERSION:2.1\n\
N;ENCODING=QUOTED-PRINTABLE;CHARSET=utf-8:%s"/*A=E5=BC=A0=E4=BA=91=E6=B8=85*/";;;;\n\
TEL;VOICE;PREF:%s"/*10086*/"\n\
END:VCARD\n\
";
void vcf_format(FILE *out, unsigned char *str)
{
	string name,number;
	char tmp[10];
	while(*str) {
		if(*str == ','){
			str++;
			break;
		}
		if(*str > 0x7f){
			sprintf(tmp, "%X", (int)*str);
			//puts(tmp);
			name += '=';
			name += tmp;
		}else{
			name += *str;
		}
		str++;
	}
	while(*str) {
		if(*str < ' ')
			break;
		number += *str;
		str++;
	}
	printf("%s------%s\n",name.c_str(), number.c_str());
	fprintf(out, vcf_fmt, name.c_str(), number.c_str());
}
int main()
{
	FILE *fin = fopen("MERGE.csv","r");
	int cnt = 0;
	while(fgets(buf, 1024, fin), !feof(fin)) {
		if(strlen(buf)>5){
			cnt++;
			//puts(buf);
			
			char name[60];
			sprintf(name, "./merged/card%d.vcf", cnt);
			FILE *fout = fopen(name,"w");
			vcf_format(fout, (unsigned char *)buf);
			fclose(fout);

		}
	}
}