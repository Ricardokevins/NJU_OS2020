#include "lib.h"
#define NULL 0
#include "types.h"

int stringLen (const char *string) 
{
    int i = 0;
    if (string == NULL)
        return 0;
    while (string[i] != 0)
        i ++;
    return i;
}
int stringCpy (const char *srcString, char *destString, int size) 
{
    int i = 0;
    if (srcString == NULL || destString == NULL)
        return -1;
    while (i != size) {
        if (srcString[i] != 0) {
            destString[i] = srcString[i];
            i++;
        }
        else
            break;
    }
    destString[i] = 0;
    return 0;
}

int stringCmp (const char *srcString, const char *destString, int size) { // compre first 'size' bytes
    int i = 0;
    if (srcString == NULL || destString == NULL)
        return -1;
    while (i != size) {
        if (srcString[i] != destString[i])
            return -1;
        else if (srcString[i] == 0)
            return 0;
        else
            i ++;
    }
    return 0;
}

int setBuffer (uint8_t *buffer, int size, uint8_t value) 
{
    int i = 0;
    if (buffer == NULL)
        return -1;
    for (i = 0; i < size ; i ++)
        buffer[i] = value;
    return 0;
}

int stringChrR (const char *string, char token, int *size) 
{
    int i = 0;
    if (string == NULL) {
        *size = 0;
        return -1;
    }
    while (string[i] != 0)
        i ++;
    *size = i;
    while (i > -1) {
        if (token == string[i]) {
            *size = i;
            return 0;
        }
        else
            i --;
    }
    return -1;
}
   
int ls(char *destFilePath) 
{
	//printf("LS %s\n", destFilePath);
	char res[1024];
	//return 1;
	do_ls(destFilePath, res);
	printf("%s\n", res);
	return 0;
}

int cat(char *cur_path,char *str) 
{
    int size=0;
    int length=stringLen(str);
    stringChrR(str,' ',&size);
    char filepath[100]={0};  
    stringCpy(cur_path,filepath,stringLen(cur_path)); 
    filepath[stringLen(cur_path)]='/';  
    stringCpy(str+size+1,filepath+stringLen(cur_path)+1,length-size-1);
    //printf("%s\n",filepath);
	char buf[1024]={0};
	int fd = open(filepath, O_READ);
	
	int ret=read(fd, (uint8_t *)buf, 1024);
	while (ret != -1)
	{
		printf("%s", buf);
		ret = read(fd, (uint8_t *)buf, 1024);
	}
	printf("\n");
	close(fd);
	return 0;
}

int cd(char* cur_path,char* str)
{
    char temp_path[1024]={0};
    stringCpy(cur_path,temp_path,128);
    int point_flag=0;
    for(int i=0;i<stringLen(str);i++)
    {
        if(str[i]==' ')
        {
            point_flag=1;
        }
        else
        {
            if(str[i]!=' '&&point_flag==1)
            {
                point_flag=i;
                break;
            }
        }
    }
    char next_path[100]={0};
    next_path[0] = '/';
    stringCpy(str+point_flag,next_path+1,100);

    int cur_index=stringLen(cur_path);
    if(cur_path[cur_index-1]=='/')
    {
        cur_path[cur_index]=0;
        cur_index--;
    }
    stringCpy(next_path,temp_path+cur_index,128);

    char temp[100];
    int ret=do_ls(temp_path,temp);
    //printf("%d %s\n",stringLen(temp_path),temp_path);
    if(ret==-1)
    {
        printf("Failed because of unexsit path\n");
        return -1;
    }
    else
    {
        //printf("success enter new dir: %s    %s\n",temp_path,next_path);
        if(stringCmp("..",next_path+1,stringLen(".."))==0)
        {
            int length;
            //int cond;
            int size=0;
            length = stringLen(cur_path);
            if (cur_path[length - 1] == '/') 
            {
                //cond = 1;
                //*((char*)cur_path + length - 1) = 0;
            }
            ret = stringChrR(cur_path, '/', &size);
            //printf("%d    %s\n",size,cur_path);
            if (ret == -1) 
            {
                printf("Incorrect destination file path.\n");
                return -1;
            }
            //char tmp = *((char*)temp_path + size + 1);
            if(size!=0)
            {
                *((char*)temp_path + size ) = 0;
                stringCpy(temp_path,cur_path,size);
                //printf("%d %s\n",stringLen(cur_path),cur_path);
            }
            else
            {
                stringCpy("/",cur_path,stringLen("/"));
            }          
            
        }
        else
        {
            if(stringCmp(".",next_path+1,stringLen("."))==0)
            {
                //printf("%d %s\n",stringLen(cur_path),cur_path);
                return 1;

            }
            else
            {
                //printf("%d %s\n",stringLen(cur_path),cur_path);
                stringCpy(temp_path,cur_path,128);
                return 1;
            }   
        }     
        return 1;
    }
    
}

int pwd(char *temp_path)
{
    printf("Current Path is : %s\n",temp_path); 
    return 1;
}

int rm(char *cur_path,char *str)
{
    int size=0;
    int length=stringLen(str);
    stringChrR(str,' ',&size);
    char filepath[100]={0};  
    stringCpy(cur_path,filepath,stringLen(cur_path)); 
    filepath[stringLen(cur_path)]='/';  
    stringCpy(str+size+1,filepath+stringLen(cur_path)+1,length-size-1);
    //printf("%s\n",filepath);
    remove(filepath);
    //printf("%d\n",ret);
    return 1;
}



/*
echo "Raspberry" > test.txt  
echo "Intel Galileo" >> test.txt  
*/

int echo(char *str,char *cur_path)
{
    int flag=0;
    int length=stringLen(str);
    for(int i=0;i<length;i++)
    {
        if(str[i]=='>'&&str[i+1]!='>')
        {
            flag=1;
            break;
        }
        if(str[i]=='>'&&str[i+1]=='>')
        {
            flag=2;
            break;
        }  
    }

    if(flag==1)
    {
        int size=0;
        stringChrR(str,'>',&size);
        char filepath[100]={0};  
        stringCpy(cur_path,filepath,stringLen(cur_path)); 
        filepath[stringLen(cur_path)]='/';  
        stringCpy(str+size+2,filepath+stringLen(cur_path)+1,length-size-2);
       // printf("%s %s %d %d\n",str+size+2,filepath,length-size-1,size);
        //printf("%d %s \n",stringLen(filepath),filepath);
        char content[100]={0};
        int left_pos=0;
        int right_pos=0;
        stringChrR(str,'"',&right_pos);
        str[right_pos]=' ';
        stringChrR(str,'"',&left_pos);
        //printf("%d %d \n",left_pos,right_pos);
        stringCpy(str+left_pos+1,content,right_pos-left_pos-1);
        //printf("%s %d %d\n",content,stringLen(content),right_pos-left_pos-1);
            
        int fd = open(filepath, O_WRITE | O_READ | O_CREATE);
        lseek(fd,0,0);
	    for (int i = 0; i < stringLen(content); i ++) 
        {
		    char tmp = content[i];
		    write(fd, (uint8_t*)&tmp, 1);
	    }
	    close(fd);
        return 1;
    }
    if(flag ==2 )
    {
        int size=0;
        stringChrR(str,'>',&size);
        char filepath[100]={0};  
        stringCpy(cur_path,filepath,stringLen(cur_path)); 
        filepath[stringLen(cur_path)]='/';  
        stringCpy(str+size+2,filepath+stringLen(cur_path)+1,length-size-2);
    
        char content[1000]={0};
        int left_pos=0;
        int right_pos=0;
        stringChrR(str,'"',&right_pos);
        str[right_pos]=' ';
        stringChrR(str,'"',&left_pos);

        int fd = open(filepath, O_WRITE | O_READ | O_CREATE);
        //read(fd, (uint8_t *)content, 1024);
        //printf("Content %s\n",content);

        stringCpy(str+left_pos+1,content,right_pos-left_pos-1);

       // printf("Content %s\n",content);
        lseek(fd,0,2);
	    for (int i = 0; i < stringLen(content); i ++) 
        {
		    char tmp = content[i];
		    write(fd, (uint8_t*)&tmp, 1);
	    }
	    close(fd);
        return 1;
    }
    return 1;

}
 
int main(void) 
{
    char str[1024]={0};
    char cur_path[1024]={0};
    stringCpy("/usr",cur_path,128);
    printf("%d\n",stringLen("jj"));
    printf("> Welcome to  181860077 kevinpro's  Shell \n");
    pwd(cur_path);
    while(1)
    {
        char ch=0;
        int index=0;
        printf("> ");
        scanf("%c", &ch);
        while(ch!='\n')
        {
            printf("%c",ch);   
            str[index]=ch;
            index++;
            scanf("%c", &ch);             
        }
        str[index]=0;
        printf("\n");
        if(stringCmp("ls",str,stringLen("ls"))==0)
        {
            ls(cur_path);
            continue;
        }
        if(stringCmp("cd",str,stringLen("cd"))==0)
        {     
            cd(cur_path,str);
            continue;
        }
        if(stringCmp("pwd",str,stringLen("pwd"))==0)
        {     
            pwd(cur_path);
            continue;
        }
        if(stringCmp("echo",str,stringLen("echo"))==0)
        {     
            echo(str,cur_path);
            continue;
        }
        if(stringCmp("cat",str,stringLen("cat"))==0)
        {     
            cat(cur_path,str);
            continue;
        }
        if(stringCmp("rm",str,stringLen("rm"))==0)
        {     
            rm(cur_path,str);
            continue;
        }
        if(stringCmp("quit",str,stringLen("quit"))==0)
        {
            break;
        }
        else
        {
            printf("unkown command\n");
        }
        
    }
    printf("Quit shell\n");
	exit();
	return 0;
}
