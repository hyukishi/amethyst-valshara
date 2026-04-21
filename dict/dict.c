#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define TRUE   1
#define FALSE  0

#define WORDLEN 50

int word_in_dict(char *word)
{
    FILE *fp;
    char filen[10],s[WORDLEN],t[WORDLEN];
    
    if (!*word)
        return(FALSE);
    
    if (strlen(word)<=2)
        return(FALSE);
    
    sprintf(filen,"%c/%c%c.txt",
            tolower(word[0]),tolower(word[0]),tolower(word[1]));
    
    if (!(fp=fopen(filen,"r")))
        return(FALSE);
    
    while (!feof(fp))
    {
        fgets(s,WORDLEN,fp);
        sscanf(s,"%s\n",t);
        if (t[2]<word[2])
            continue;
        else if (t[2]>word[2])
            break;
        if (!strcmp(word,t))
        {
            fclose(fp);
            return(TRUE);
        }
    }
    
    fclose(fp);
    return(FALSE);
}

#define CHECK(s) printf("%s: %d\n",(s),word_in_dict(s));

int main(int argc, char *argv[])
{
    FILE *dictfile,*fp;
    char s[WORDLEN],t[WORDLEN];
    int x;
    
    t[0]='\0';
    s[0]='\0';
    
    if (!(dictfile=fopen("dict.txt","r")))
    {
        printf("Unable to open dict.txt.\n\n");
        return 1;
    }
    
    if (argc==2)
        while (!feof(dictfile))
        {
            fgets(s,WORDLEN,dictfile);
            sscanf(s,"%s\n",t);
            if (strlen(t)<=3)
                continue;
            for (x=0;x<strlen(t);x++)
                t[x] = tolower(t[x]);
            sprintf(s,"%c/%c%c.txt",t[0],t[0],t[1]);
            if (!(fp=fopen(s,"a")))
            {
                printf("Unable to append %s.\n\n",s);
                continue;
            }
            fprintf(fp,"%s\n",t);
            fclose(fp);
        }
    
    fclose(dictfile);
    
    CHECK("test");
    CHECK("geek");
    CHECK("corn");
    CHECK("cruel");
    CHECK("race");
    CHECK("hacker");
    CHECK("gilgamesh");
    CHECK("greed");
    CHECK("pinetop");
    CHECK("altruistic");
    CHECK("captain");
    CHECK("destiny");
    CHECK("can");
    CHECK("cork");
    CHECK("cold");
    CHECK("cod");
    CHECK("coach");
    CHECK("cozy");
    CHECK("caelum");

    return 0;
}
