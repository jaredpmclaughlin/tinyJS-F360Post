/*
 * TinyJS
 *
 * A single-file Javascript-alike engine
 *
 * Authored By Gordon Williams <gw@pur3.co.uk>
 *
 * Copyright (C) 2009 Pur3 Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * This is a simple program showing how to use TinyJS
 */

#include "TinyJS.h"
#include "TinyJS_Functions.h"
#include "TinyJS_MathFunctions.h"
#include "TinyJS_Additional.h"
#include "Fusion_Functions.h"
#include <assert.h>
#include <stdio.h>


const char *code = "function myfunc(x, y) { return x + y; } var a = myfunc(1,2); print(a);";
const size_t readBufSz = 2048;

void js_print(CScriptVar *v, void *userdata);
void js_dump(CScriptVar *v, void *userdata);
void eval_script(CTinyJS *js, char *filename);

int main(int argc, char **argv)
{
  CTinyJS *js = new CTinyJS();
  /* add the functions from TinyJS_Functions.cpp */
  registerFunctions(js);
  /* add the math functions from TinyJS_MathFunctions.cpp */
  registerMathFunctions(js);
  /* add the functions from TinyJS_Additional.cpp */
  registerAdditionalFunctions(js);
  /* add the functions from Fusion_Functions.cpp */
  registerFusionFunctions(js);
  
  /* Add a native function */
  js->addNative("function print(text)", &js_print, 0);
  js->addNative("function dump()", &js_dump, js);

  /* Open our output file */
  try {
    fileOpen("out.nc");
  } catch (CScriptException *e) {
    printf("ERROR: %s\n", e->text.c_str());
  }

  eval_script(js,"test_data/rs274.cps");

  /* Execute out bit of code - we could call 'evaluate' here if
     we wanted something returned */
  try {
    js->execute("var lets_quit = 0; function quit() { lets_quit = 1; }");
    js->execute("print(\"Interactive mode... Type quit(); to exit, or print(...); to print something, or dump() to dump the symbol table!\");");
  } catch (CScriptException *e) {
    printf("ERROR: %s\n", e->text.c_str());
  }

  /* REPL */
  while (js->evaluate("lets_quit") == "0") {
    char buffer[readBufSz];
    fgets ( buffer, readBufSz, stdin );
    try {
      js->execute(buffer);
    } catch (CScriptException *e) {
      printf("ERROR: %s\n", e->text.c_str());
    }
  }
  delete js;
  fileClose();
#ifdef _WIN32
#ifdef _DEBUG
  _CrtDumpMemoryLeaks();
#endif
#endif
  return 0;
}


void js_print(CScriptVar *v, void *userdata) {
    printf("> %s\n", v->getParameter("text")->getString().c_str());
}

void js_dump(CScriptVar *v, void *userdata) {
    CTinyJS *js = (CTinyJS*)userdata;
    js->root->trace(">  ");
}

struct block_s {
    size_t last_cap=0;
    size_t capacity=0;
    size_t size=0; 
    char* data=NULL; 
};

void block_append(struct block_s* block, char* string){
    if(block==NULL || string==NULL) return;

    size_t length = strlen(string);
	block->last_cap=block->capacity;

    if(block->capacity == 0) {
        block->capacity=length+1;
        block->data= (char *) malloc(sizeof(char)*block->capacity);
    }

    while(block->capacity < (block->size+length))
	block->capacity+=block->capacity+1;

    if(block->last_cap < block->capacity) {
        block->data=(char *)realloc(block->data, ((block->capacity)));
    }

	if(block->size)
    	memcpy(&(block->data[block->size-1]),string,length);
	else
    	memcpy(&(block->data[0]),string,length);
		
	block->size+=length;

}

void block_clear(struct block_s* block) {
//    printf("Block contents:\n%s\n", block->data);
//    printf("Block cleared.\n");
	memset(block->data,'\0',block->size);
	block->size=0;
}

void eval_script(CTinyJS* js,char* filename) {

  FILE *script = fopen(filename,"r");
  if(script==NULL) 
    throw new CScriptException(strerror(errno));

  printf("Evaluating script.\n");

  size_t blockSz=readBufSz*2;
  char buffer[readBufSz];
  struct block_s block;

  const std::string block_comment_start_string("/**");
  const std::string block_comment_end_string("*/");
  const std::string block_start_string("{");
  const std::string block_end_string("}");

  bool block_comment{false};
  bool inblock{false};

  char* comment_start=NULL;

  size_t braces{0};

  size_t line=0;
  char err_str[50];

  try {
    // fgets == NULL on eof with 0 chars read, or error
    while(fgets(buffer,readBufSz,script)!=NULL) { 
    /* js->execute has trouble with comments and newlines
     *  that occur within some blocks, so we strip them out
     */
        line++;
        if( strlen(buffer)==(readBufSz-1) && buffer[readBufSz-1]!='\0' ) {
            sprintf(err_str, "Script line %u exceeds buffer length.");
            throw new CScriptException(err_str);
        }

        if((comment_start=strstr(buffer,"/**"))!=NULL  && ! block_comment) {
            (*comment_start)='\0';
            block_comment=true;
        } else if(strstr(buffer,"*/")!=NULL && block_comment) {
            block_comment=false;
            buffer[0]='\0'; 
        }

        if(strstr(buffer,"{")!=NULL && strstr(buffer,"}")==NULL){
            do {
                for(size_t i=0; i<strlen(buffer); i++) {
                    if(buffer[i]=='{') braces++;
                    if(buffer[i]=='}') braces--;
                    if(buffer[i]=='\n' || buffer[i]=='\r')
                        buffer[i]=' ';
                }
                block_append(&block,buffer);
                fgets(buffer,readBufSz,script);
            } while(braces);
            js->execute(block.data);
            block_clear(&block);
            buffer[0]='\0'; 
        }

        if(strstr(buffer,"[")!=NULL && strstr(buffer,"]")==NULL){
            do {
                for(size_t i=0; i<strlen(buffer); i++) {
                    if(buffer[i]=='[') braces++;
                    if(buffer[i]==']') braces--;
                    if(buffer[i]=='\n' || buffer[i]=='\r')
                        buffer[i]=' ';
                }
                block_append(&block,buffer);
                fgets(buffer,readBufSz,script);
            } while(braces);
            js->execute(block.data);
            block_clear(&block);
            buffer[0]='\0'; 
        }

        if(!block_comment)
            { js->execute(buffer); }
    }
  } catch (CScriptException *e) {
        printf("Error Reading Script: %s\n", e->text.c_str());
        printf("Current buffer: %s\n", buffer);
        printf("Current block: %s\n", block.data);
  }
}
