//csv��ʽͨѶ¼תvcard�ɰ��ʽ(��ŵ����,outlook)
//csv�ļ��� MERGE.csv ,�ޱ���,ÿ��һ��,��ʽ "����,����"
//vcard�ļ���./merged/*.vcf ,ÿ��һ���ļ�

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