#include <stdio.h>
#include <stdlib.h>
#include <math.h>
typedef struct
{
    unsigned char b,g,r;
}pixel;
typedef union{
    unsigned int x;
    unsigned char octeti[4];
}u_int;
unsigned int* XORSHIFT32(unsigned int seed,unsigned int n)
{
    unsigned int r=seed,i;
    unsigned int *R=malloc(sizeof(unsigned int)*n); //creem numere aleatoare pornid de la parametrul seed si le punem in vectorul dinamic de dimensine n
    for(i=0;i<n;i++)
    {
        r=r^r<<13;
        r=r^r>>17;
        r=r^r<<5;
        R[i]=r;
    }
    return R;
}
pixel* liniarizare(const char *calea_imaginii,unsigned char **header,unsigned int *pixelnr)
{
    unsigned int w,h;
    FILE *fin;
    fin=fopen(calea_imaginii,"rb");
    if(fin==NULL){
            printf("eroare la deschiderea fisierului\n");
            return 0;}
        int i;
        for(i=0;i<54;i++)
        {
            int x;
            fread(&x,1,1,fin);//salvam informatiile din header
            (*header)[i]=x;
        }
        rewind(fin);//ne intoarcem la inceputul fisierului
        fseek(fin,18,SEEK_SET);//pozitionam pointelul pe 18 pt a afla dimensiunile imaginii
        fread(&w,4,1,fin);//aflam latimea
        fread(&h,4,1,fin);//aflam inaltimea

        int nr_pad=4-(3*w)%4; //calculam nr de octetii de padding
        if(nr_pad==4) nr_pad=0;
        pixel *L;
        L=malloc((w*h-1)*sizeof(pixel)); //alocam memorie pentru liniarizare
         fseek(fin,54,SEEK_SET); //deplasam pointerul dupa header
        int j;
        for(i=0;i<h;i++){
            for(j=0;j<w;j++)
        {
            pixel p;
            fread(&p.r,1,1,fin);//citim 3 octeti corespunzatori unui pixel si in vectorul L
            fread(&p.g,1,1,fin);
            fread(&p.b,1,1,fin);
            L[i*w+j]=p;
        }
        fseek(fin,nr_pad,SEEK_CUR);//deplasam pointerul cu nr de octeti de padding
        }
        fclose(fin);
        (*pixelnr)=w*h;
        //intoarcem imaginea???
        //for(i=0;i<h/2;i++)
            //for(j=0;j<w;j++)
                //swap(L[i*w+j],L[(h-1-i)*w+j]);

    return L;
}
void salvare(char *calea_salvarii,unsigned char *header,pixel *L)
{
    FILE *fout=fopen(calea_salvarii,"wb+");
    if(fout==NULL) printf("Eroare la deschderea fisierului");
    int i;
    for(i=0;i<54;i++)
    {
        unsigned char x=header[0];
        fwrite(&x,1,1,fout); //se scrie continutul tabloului header care ca reprezenta headerul imaginii noi
    }
    rewind(fout);
    fseek(fout,18,SEEK_SET);
    unsigned int w,h;
    fread(&w,sizeof(unsigned int),1,fout);
    fread(&h,4,1,fout);//?? //se afla dimensiunile imaginii
    int nr_pad=4-(3*w)%4; //calculam nr de octetii de padding
    if(nr_pad==4) nr_pad=0;
    int j;
    for(i=0;i<h;i++){
        for(j=0;j<w;j++)
        {
            pixel p=L[i*w+j];
            fwrite(&p.b,1,1,fout);
            fwrite(&p.g,1,1,fout);
            fwrite(&p.r,1,1,fout);//????????????nuj daca trebuie invers

        }
        unsigned char *v=calloc(nr_pad,sizeof(unsigned char));
        fwrite(v,nr_pad,1,fout);//scriem paddingul
    }
    fclose(fout);

}
pixel pxorp(pixel p1,pixel p2)
{
    p1.b=p1.b^p2.b;
    p1.g=p1.g^p2.g;
    p1.r=p1.r^p2.r;
    return p1;
}
pixel pxoru(pixel p,unsigned int u)
{
    u_int aux;
    aux.x=u;
    p.b=p.b^aux.octeti[0];
    p.g=p.g^aux.octeti[1];
    p.r=p.r^aux.octeti[2];
    return p;
}
unsigned int *perm_aleatoare(unsigned int n,unsigned int *R)
{
    unsigned int *perm=malloc((n-1)*sizeof(unsigned int));//alocam spatiu pentru permutare
    int i;
    for(i=0;i<n-1;i++)//construim permutarea identica
        perm[i]=i+1;
    for(i=n-1;i>=1;i--)//se porneste de la ultima pozitie
    {
        int r=R[n-1-i]%(i+1);//elem se transpune cu nr aflat pe poz w*h-1-i in vectorul cu nr random
        unsigned int aux=perm[r]; //se ia modulo i+1 pt ca r sa fie in intervalul (0,i)
        perm[r]=perm[i];
        perm[i]=aux;

    }
    return perm;

}
void criptare(char *calea_img_initiale,char *calea_img_finale,char *calea_cheii)
{
    unsigned char *header;
    unsigned int pixelnr;
    pixel *L=liniarizare(calea_img_initiale,&header,&pixelnr);
    FILE *fkey=fopen(calea_cheii,"r");
    unsigned int R0,SV;
    fscanf(fkey,"%d%d",&R0,&SV);
    unsigned int *rand=XORSHIFT32(R0,2*pixelnr-1);//?? se aloca momeorie pentru vecorul de nr random
    unsigned int *perma=perm_aleatoare(pixelnr,rand);//construim permutarea aleatoare
    pixel *aux=malloc((pixelnr-1)*sizeof(pixel));//se aloca memorie pt aplicarea permutarii perm asupra tabloului L
    int i;
    for(i=0;i<pixelnr-1;i++)
        aux[i]=L[perma[i]];
    aux[0]=pxoru(pxoru(aux[0],SV),rand[pixelnr]);
    for(i=1;i<pixelnr-1;i++)
        aux[i]=pxoru(pxorp(aux[i],aux[i-1]),rand[pixelnr+i]);
    salvare(calea_img_finale,header,aux);
    fclose(fkey);//eliberam memoria si inchidem fisiererle
    free(L);
    free(header);
    free(rand);
    free(perma);
    free(aux);
}
void decriptare(char *calea_img_initiale,char *calea_img_decriptate,char *calea_cheii)
{
    unsigned char *header;
    unsigned int pixnr;
    pixel *C=liniarizare(calea_img_initiale,&header,&pixnr);
    FILE *fkey=fopen(calea_cheii,"r");
    unsigned int R0,SV;
    fscanf(fkey,"%d%d",&R0,&SV);
    fclose(fkey);
    unsigned int *rand=XORSHIFT32(R0,2*pixnr-1);//generam secv de nr intregi aleTOARE
    unsigned int *perma=perm_aleatoare(pixnr,rand);//GENERAM PERM ALEATOARE
    unsigned int *perminv=malloc(pixnr*sizeof(unsigned int));
    int i; //calculam permutarea inversa
    for(i=0;i<pixnr-1;i++)
        perminv[perma[i]-1]=i+1;
    pixel *CC=malloc((pixnr-1)*sizeof(pixel));//aplicam fiecarui pixel relatia inversa de substitutie
    CC[0]=pxoru(pxoru(C[0],SV),rand[pixnr]);
    for(i=1;i<pixnr-1;i++)
    {
        CC[i]=pxoru(pxorp(CC[i-1],CC[i]),rand[pixnr+i]);
    }
    pixel *D=malloc((pixnr-1)*sizeof(pixel));
    for(i=0;i<pixnr-1;i++)//obtinem imaginea decriptata permutand pixelii din cc conform premutarii inverse
        D[perminv[i]]=CC[i];
    salvare(calea_img_decriptate,header,D);
    free(C); free(CC); free(D); free(rand);free(perma);free(perminv); //eliberam memoria
    free(header);

}
float media_canalului(unsigned int *Frec,float fmed)
{
    float S=0;
    int i;
    for(i=0;i<=255;i++)
        S+=(Frec[i]-fmed)*(Frec[i]-fmed);
    S=S/fmed;
    return S;
}
void chiSquare(char *calea_imaginii)
{
    unsigned char *header;
    unsigned int pixnr,i;
    pixel *L=liniarizare(calea_imaginii,&header,&pixnr);
    float fmed=pixnr/256;//se calculeaza media presupusa
    unsigned int *Rfrecv=calloc(255,sizeof(unsigned int));//se aloca tablourile ce vor stoca frecventa pe fiecare canal
    unsigned int *Gfrecv=calloc(255,sizeof(unsigned int));
    unsigned int *Bfrecv=calloc(255,sizeof(unsigned int));
    for(i=0;i<pixnr-1;i++)
    {
        Rfrecv[L[i].r]++;//se parcurge tabloul L si se afla frecventa valorilor pe fiecare canal
        Gfrecv[L[i].g]++;
        Bfrecv[L[i].b]++;
    }
    float Sr=media_canalului(Rfrecv,fmed);//se calculeaza media reala a valorilor
    float Sg=media_canalului(Gfrecv,fmed);
    float Sb=media_canalului(Bfrecv,fmed);
    printf("media canalului rosu:%.2f\n",Sr);//se afiseaza pe ecran cele 3 valori
    printf("media canalului verde:%.2f\n",Sg);
    printf("media canalului albastru:%.2f\n",Sb);


}
typedef struct{
    struct{
    unsigned int l,c;
    }stanga_sus,dreapta_jos;
    unsigned int aria;
    double corelatia;
    pixel culoare;
}detectie;
void grayscale_image(char* nume_fisier_sursa,char* nume_fisier_destinatie)
{
   FILE *fin, *fout;
   unsigned int dim_img, latime_img, inaltime_img;
   unsigned char pRGB[3],aux;


   fin = fopen(nume_fisier_sursa, "rb+");
   if(fin == NULL)
   	{
   		printf("nu am gasit imaginea sursa din care citesc");
   		return;
   	}

   fout = fopen(nume_fisier_destinatie, "wb+");

   fseek(fin, 2, SEEK_SET);
   fread(&dim_img, sizeof(unsigned int), 1, fin);

   fseek(fin, 18, SEEK_SET);
   fread(&latime_img, sizeof(unsigned int), 1, fin);
   fread(&inaltime_img, sizeof(unsigned int), 1, fin);

   //copiaza octet cu octet imaginea initiala in cea noua
	fseek(fin,0,SEEK_SET);
	unsigned char c;
	while(fread(&c,1,1,fin)==1)
	{
		fwrite(&c,1,1,fout);
		fflush(fout);
	}
	fclose(fin);

	//calculam padding-ul pentru o linie
	int padding;
    if(latime_img % 4 != 0)
        padding = 4 - (3 * latime_img) % 4;
    else
        padding = 0;

    printf("padding = %d \n",padding);

	fseek(fout, 54, SEEK_SET);
	int i,j;
	for(i = 0; i < inaltime_img; i++)
	{
		for(j = 0; j < latime_img; j++)
		{
			//citesc culorile pixelului
			fread(pRGB, 3, 1, fout);
			//fac conversia in pixel gri
			aux = 0.299*pRGB[2] + 0.587*pRGB[1] + 0.114*pRGB[0];
			pRGB[0] = pRGB[1] = pRGB[2] = aux;
        	fseek(fout, -3, SEEK_CUR);
        	fwrite(pRGB, 3, 1, fout);
        	fflush(fout);
		}
		fseek(fout,padding,SEEK_CUR);
	}
	fclose(fout);
}
pixel **incarcare(char *calea_img,unsigned char **header,unsigned int *wi,unsigned int *hi)
{

    int i,j;
    unsigned int w,h;
    FILE *fin=fopen(calea_img,"rb");
    (*header)=malloc(54*sizeof(unsigned char));
    for(i=0;i<54;i++)
        {
            int x;
            fread(&x,1,1,fin);//salvam informatiile din header
            (*header)[i]=x;
        }
    rewind(fin);
    fseek(fin,18,SEEK_SET);//pozitionam pointelul pe 18 pt a afla dimensiunile imaginii
    fread(&w,sizeof(unsigned int),1,fin);//aflam latimea
    fread(&h,sizeof(unsigned int),1,fin);//aflam inaltimea

    unsigned int nr_pad=4-(3*w)%4; //calculam nr de octetii de padding
    if(nr_pad==4)
        nr_pad=0;

    pixel **map=malloc(h*sizeof(pixel*));//contine adrese catre fiecare linie a imaginii
    for(i=0;i<h;i++)
    {
        map[i]=malloc(w*sizeof(pixel));//se aloca fiecare adressa ca si tablou de pixeli
    }
    for(i=0;i<h;i++){
            for(j=0;j<w;j++)
        {
            pixel p;
            fread(&p.r,1,1,fin);//citim 3 octeti corespunzatori unui pixel
            fread(&p.g,1,1,fin);
            fread(&p.b,1,1,fin);
            map[i][j]=p;//parcurgem imaginea pixel cu pixel si se retine pixelul curent in tablou  pe pozitia corespunzatoate
        }
         fseek(fin,nr_pad,SEEK_CUR);
         }
         fclose(fin);
         (*wi)=w;
         (*hi)=h;

    return map;
}
int cmp(const void *a,const void *b)
{
    detectie *va=(detectie *)a;
    detectie *vb=(detectie *)b;
    if(vb->corelatia>va->corelatia) return 1;
    return -1;
}
typedef struct{
struct{
unsigned int l,c;}stanga_sus,dreapta_jos;
}colt;

double suprapunere(detectie a,detectie b)
{
    colt maxx;
    //se calculeaza coltul din stanga sus fiind linia si coloana maxima din stanga sus dintre cele doua detectii
    if(a.stanga_sus.l>b.stanga_sus.l)
        maxx.stanga_sus.l=a.stanga_sus.l;
    else maxx.stanga_sus.l=b.stanga_sus.l;

    if(a.stanga_sus.c>b.stanga_sus.c)
        maxx.stanga_sus.c=a.stanga_sus.c;
    else maxx.stanga_sus.c=b.stanga_sus.c;

    //coltul din dr jos =minimul dr dintre detectii
    if(a.dreapta_jos.c<b.dreapta_jos.c)
        maxx.dreapta_jos.c=a.dreapta_jos.c;
    else maxx.dreapta_jos.c=b.dreapta_jos.c;

    if(a.dreapta_jos.l<b.dreapta_jos.l)
        maxx.dreapta_jos.l=a.dreapta_jos.l;
    else maxx.dreapta_jos.l=b.dreapta_jos.l;
    //daca intersectia are coltul din stanga sus mai jos sau mai la dreapta decat coltul dreapta jos atunci suprapunerea e 0
    if(maxx.stanga_sus.l>maxx.dreapta_jos.l) return 0;
    if(maxx.stanga_sus.c>maxx.dreapta_jos.c) return 0;

    //calculam aria intersestiei
    unsigned int hi,wi;
    int x=b.stanga_sus.c-a.stanga_sus.c;
    if(x<0) x=-x;
    wi=11-x;
    x=b.stanga_sus.l-a.stanga_sus.l;
    if(x<0) x=-x;
    hi=15-x;
    double aria=hi*wi;


    return aria/(330-aria);//calculam suprapunerea

}
detectie *stergeNonMax(detectie *D,unsigned int *nrdetectii)
{
    unsigned int n=(*nrdetectii);
    qsort(D,n,sizeof(detectie),cmp);//sortam vecorul descrescator folosint functia qsort
    int i,j;
    for(i=0;i<n-1;i++)
        for(j=i+1;j<n;i++)
    {
        double supr;
        supr=suprapunere(D[i],D[j]);
        if(supr>0.2)//daca elem se suprapun eliminam elementul cu scor mai mic
        {
            int z;
            for(z=j;z<n;z++)
                D[z]=D[z+1];
            n--;
        }
    }
    detectie *DD=realloc(D,n*sizeof(detectie));
    (*nrdetectii)=n;
    return DD;
}
void corelatie(pixel **pixelimagine, pixel **pixelsablon,detectie *contur,unsigned int ws,unsigned int hs)
{
    int i,j;
    unsigned int ns=hs*ws;
    unsigned int x,y;
    x=(*contur).stanga_sus.l;
    y=(*contur).stanga_sus.c;
    double cor,Smed=0,Fmed=0;
    for(i=0;i<hs;i++)//parcurgem sablonul si pt a afla media val intensitatilor grayscale
        for(j=0;j<ws;j++)
            {
                Smed+=pixelsablon[i][j].r;
                Fmed+=pixelimagine[i+x][j+y].r;
            }
    Smed=Smed/ns;
    Fmed=Fmed/ns;
    double sigmaS=0, sigmaF=0;
    for(i=0;i<hs;i++)
        for(j=0;j<ws;j++)
    {
        sigmaS+=(pixelsablon[i][j].r-Smed)*(pixelsablon[i][j].r-Smed);//calculam deviatiile
        sigmaF+=(pixelimagine[x+i][y+j].r-Fmed)*(pixelimagine[x+i][y+j].r-Fmed);//x=linia,y=coloana
    }
    double raport=ns-1;
    raport=1/raport;
    sigmaS=sigmaS*raport;
    sigmaS=sqrt(sigmaS);
    sigmaF=sigmaF*raport;
    sigmaF=sqrt(sigmaF);
    raport=sigmaF*sigmaS;
    raport=1/raport;
    for(i=0;i<hs;i++)
        for(j=0;j<ws;j++)
    {
        cor+=(pixelimagine[x+i][y+j].r-Fmed)*(pixelsablon[i][j].r-Smed)*raport;//calculam suma din corelatie
    }
    cor=cor/ns;
    (*contur).corelatia=cor;//salvam corelatia in campul corelatie al variabilei contur

}
detectie *match(pixel **img,char *sablon,float prag,unsigned int *nrdetectii,pixel culoare,unsigned int w, unsigned int h,detectie *D)
{
    unsigned char *header_sabl;
    unsigned int ws,hs;
    //se incarca in memoria interna sablonul
    grayscale_image(sablon,sablon);//transforma sablonul in grayscale
    pixel **sabl=incarcare(sablon,&header_sabl,&ws,&hs);
    int i,j;
    for(i=0;i<h-hs;i++)//pargurgem imaginea mare
        for(j=0;j<w-ws;j++)
        {
            detectie contur;
            contur.stanga_sus.l=i;
            contur.stanga_sus.c=j;
            contur.dreapta_jos.l=i+hs;
            contur.dreapta_jos.c=j+ws;
            contur.culoare=culoare;//salvam culoarea cu care va trebui desenat chnarul in campul corespunzator
            corelatie(img,sabl,&contur,ws,hs);
            if(contur.corelatia>=prag)//daca corelatia este mai mare decat pragul variabila contur se adauga in tabloul D
            {
                (*nrdetectii)++;
                detectie *Dd=realloc(D,(*nrdetectii)*sizeof(detectie));//la fiecare pas se realoca cu memorie suficienta
                D=Dd;
                free(Dd);
                D[(*nrdetectii)-1]=contur;
            }
        }
    for(i=0;i<hs;i++) free(sabl[i]);
    free(sabl);
    return D;
}
void salvaremat(pixel **imagine,char *nume_cale,unsigned char *header,unsigned int w,unsigned int h)
{
    FILE *fout=fopen(nume_cale,"wb");
    int i,j;
    for(i=0;i<54;i++)
    {
        unsigned char x=header[0];
        fwrite(&x,1,1,fout); //se scrie continutul tabloului header care ca reprezenta headerul imaginii noi
    }
    int nr_pad=4-(3*w)%4; //calculam nr de octetii de padding
    if(nr_pad==4) nr_pad=0;

    for(i=0;i<h;i++){
        for(j=0;j<w;j++)
        {
            pixel p=imagine[i][j];
            fwrite(&p.b,1,1,fout);
            fwrite(&p.g,1,1,fout);
            fwrite(&p.r,1,1,fout);

        }
        unsigned char *v=calloc(nr_pad,sizeof(unsigned char));
        fwrite(v,nr_pad,1,fout);//scriem paddingul
    }
    fclose(fout);
}
pixel **colorare(pixel **imagine,detectie contur)
{
    int j;
    for(j=0;j<11;j++){
        imagine[contur.stanga_sus.l][j+contur.stanga_sus.c]=contur.culoare;
        imagine[contur.dreapta_jos.l][contur.dreapta_jos.c-j]=contur.culoare;}
    for(j=0;j<15;j++)
    {
        imagine[contur.stanga_sus.l+j][contur.stanga_sus.c]=contur.culoare;
        imagine[contur.dreapta_jos.l-j][contur.dreapta_jos.c]=contur.culoare;
    }
    return imagine;
}
void template_matching(char *initial,char *rezultat)
{
    unsigned int w,h,i;
    unsigned char *header;
    float prag;
    scanf("%f",&prag);//se citeste pragul folosit in cadrul funtiei de corelatie
    int nrsabloane=10;
    char **sablon;
    sablon=malloc(nrsabloane*sizeof(char*));//sablon=un tablou cu caile de acces spre fiecare sablon
    for(i=0;i<nrsabloane;i++)//se citesc caile de acces catre sabloane
    {
        sablon[i]=malloc(100*sizeof(char));
        scanf("%s",sablon[i]);
    }

    char img_gray[]="imagine_grayscale.bmp";//convertim imaginea gayscale si ii retinem calea
    grayscale_image(initial,img_gray);
    pixel **img=incarcare(img_gray,&header,&w,&h);
    detectie *D=malloc(1*sizeof(detectie));//d va retine chenarele a calor corelatie cu imaginea plansa este mai mare sau egala cu praguk
    pixel *culoare=malloc(sizeof(pixel)*nrsabloane); //se aloca u tablou de culori corespunzator culorilor din enunt
    for(i=0;i<nrsabloane;i++)
    {
        pixel p;
        scanf("%c%c%c",&p.r,&p.g,&p.b);//citim culorile corepunzatoare sabloanelor
        culoare[i]=p;
    }
    unsigned int nrdetectii=0;
    for(i=0;i<nrsabloane;i++)
    {
        D=match(img,sablon[i],prag,&nrdetectii,culoare[i],w,h,D);//apelam funtia match pt fiecare sablon in parte
    }
    D=stergeNonMax(D,&nrdetectii);
    pixel **img_originala=incarcare(initial,&header,&w,&h);//se incarca in memorie imaginea originala
    for(i=0;i<nrdetectii;i++)//parcurgem tabloul D si coloram fiecare detectie cu un chenar in cadrul imaginii originale
    {
        img_originala=colorare(img_originala,D[i]);

    }
    salvaremat(img_originala,rezultat,header,w,h);
    free(D);
    for(i=0;i<nrsabloane;i++) free(sablon[i]);
    free(sablon);
    free(culoare);
    for(i=0;i<h;i++) free(img_originala[i]);
    free(img_originala);
    for(i=0;i<h;i++) free(img[i]);
    free(img);

}
int main()
{
    char *cale_ic=malloc(101*sizeof(char));//citim caile pentru imagini si fiserul cheie
    scanf("%s",cale_ic);
    char *cale_fc=malloc(101*sizeof(char));
    scanf("%s",cale_fc);
    char *cale_keyc=malloc(101*sizeof(char));
    scanf("%s",cale_keyc);
    criptare(cale_ic,cale_fc,cale_keyc);
    free(cale_keyc);
    char *cale_id=malloc(101*sizeof(char));
    scanf("%s",cale_id);
    char *cale_fd=malloc(101*sizeof(char));
    scanf("%s",cale_fd);
    char *cale_keyd=malloc(101*sizeof(char));
    scanf("%s",cale_keyd);
    decriptare(cale_id,cale_fd,cale_keyd);
    free(cale_id); free(cale_fd),free(cale_keyd);

    printf("Testul X^2 pentru imaginea initiala:\n");//aflam valorile testului x^2 pt imaginea initiala si criptata
    chiSquare(cale_ic);
    printf("Testul C^2 pentru imaginea criptata:\n");
    chiSquare(cale_fc);
    free(cale_ic); free(cale_fc);
    char *cale_tm=malloc(101*sizeof(char));//citim imaginea pt template matching
    scanf("%s",cale_tm);
    char *cale_finala=malloc(101*sizeof(char));
    scanf("%s",cale_finala);
    template_matching(cale_tm,cale_finala);
    free(cale_tm);
    free(cale_finala);
    return 0;
}
