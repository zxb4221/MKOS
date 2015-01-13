int strcmp_s(const char* s1, const char* s2,unsigned int s)
{
    const char* p1 = s1;
    const char* p2 = s2;
    int i;
    for(i = 0; i < s; ++i)
    {
    	if(*p1 != *p2)
    		 return (*p1 - *p2);
    	else if(*p1 == 0)
    		break;
    	p1++;
    	p2++;
    }
    return 0;

}

int strcmp(const char* s1, const char* s2)
{
    const char* p1 = s1;
    const char* p2 = s2;
    if(p1 == 0 || p2 == 0)
        return (p1 - p2);

    for(; *p1 && *p2; p1++, p2++)
    {
        if(*p1 != *p2)
            break;
    }
    return (*p1 - *p2);

}
