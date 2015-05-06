/*************************************************************************/
/*  dir_access_jandroid.cpp                                              */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#ifndef ANDROID_NATIVE_ACTIVITY

#include "dir_access_jandroid.h"
#include "file_access_jandroid.h"
#include "thread_jandroid.h"
jobject DirAccessJAndroid::io=NULL;
jclass DirAccessJAndroid::cls=NULL;
jmethodID DirAccessJAndroid::_dir_open=NULL;
jmethodID DirAccessJAndroid::_dir_next=NULL;
jmethodID DirAccessJAndroid::_dir_close=NULL;


DirAccess *DirAccessJAndroid::create_fs() {

	return memnew(DirAccessJAndroid);
}

bool DirAccessJAndroid::list_dir_begin() {

	list_dir_end();
	JNIEnv *env = ThreadAndroid::get_env();

	jstring js = env->NewStringUTF(current_dir.utf8().get_data());
	int res = env->CallIntMethod(io,_dir_open,js);
	if (res<=0)
		return true;

	id=res;

	return false;
}

String DirAccessJAndroid::get_next(){

	ERR_FAIL_COND_V(id==0,"");

	JNIEnv *env = ThreadAndroid::get_env();
	jstring str= (jstring)env->CallObjectMethod(io,_dir_next,id);
	if (!str)
		return "";

	int sl = env->GetStringLength(str);
	if (sl==0) {
		env->DeleteLocalRef((jobject)str);
		return "";
	}

	CharString cs;
	cs.resize(sl+1);
	env->GetStringRegion(str,0,sl,(jchar*)&cs[0]);
	cs[sl]=0;

	String ret;
	ret.parse_utf8(&cs[0]);
	env->DeleteLocalRef((jobject)str);

	return ret;

}
bool DirAccessJAndroid::current_is_dir() const{

	JNIEnv *env = ThreadAndroid::get_env();
	String sd;
	if (current_dir=="")
		sd=current;
	else
		sd=current_dir+"/"+current;

	jstring js = env->NewStringUTF(sd.utf8().get_data());

	int res = env->CallIntMethod(io,_dir_open,js);
	if (res<=0)
		return false;

	env->CallObjectMethod(io,_dir_close,res);


	return true;
}
void DirAccessJAndroid::list_dir_end(){

	if (id==0)
		return;

	JNIEnv *env = ThreadAndroid::get_env();
	env->CallObjectMethod(io,_dir_close,id);
	id=0;


}

int DirAccessJAndroid::get_drive_count(){

	return 0;
}
String DirAccessJAndroid::get_drive(int p_drive){

	return "";
}

Error DirAccessJAndroid::change_dir(String p_dir){

	JNIEnv *env = ThreadAndroid::get_env();
	p_dir=p_dir.simplify_path();

	if (p_dir=="" || p_dir=="." || (p_dir==".." && current_dir==""))
		return OK;

	String new_dir;

	if (p_dir.begins_with("/"))
		new_dir=p_dir.substr(1,p_dir.length());
	else if (p_dir.begins_with("res://"))
		new_dir=p_dir.substr(6,p_dir.length());
	else  //relative
		new_dir=new_dir+"/"+p_dir;

//test if newdir exists
	new_dir=new_dir.simplify_path();

	jstring js = env->NewStringUTF(new_dir.utf8().get_data());
	int res = env->CallIntMethod(io,_dir_open,js);
	if (res<=0)
		return ERR_INVALID_PARAMETER;

	env->CallObjectMethod(io,_dir_close,res);



	return OK;
}

String DirAccessJAndroid::get_current_dir(){

	return "/"+current_dir;
}

bool DirAccessJAndroid::file_exists(String p_file){

	JNIEnv *env = ThreadAndroid::get_env();
	String sd;
	if (current_dir=="")
		sd=p_file;
	else
		sd=current_dir+"/"+p_file;

	FileAccessJAndroid *f = memnew(FileAccessJAndroid);
	bool exists = f->file_exists(sd);
	memdelete(f);

	return exists;
}

bool DirAccessJAndroid::dir_exists(String p_dir) {

	JNIEnv *env = ThreadAndroid::get_env();

	String sd;
	if (current_dir=="")
		sd=p_dir;
	else
		sd=current_dir+"/"+p_dir;

	String path=fix_path(sd).simplify_path();
	if (path.begins_with("/"))
		path=path.substr(1,path.length());
	else if (path.begins_with("res://"))
		path=path.substr(6,path.length());

	jstring js = env->NewStringUTF(path.utf8().get_data());
	int res = env->CallIntMethod(io,_dir_open,js);
	if (res<=0)
		return false;

	env->CallVoidMethod(io,_dir_close,res);
	env->DeleteLocalRef(js);

	return true;
}

Error DirAccessJAndroid::make_dir(String p_dir){

	ERR_FAIL_V(ERR_UNAVAILABLE);
}

Error DirAccessJAndroid::rename(String p_from, String p_to){

	ERR_FAIL_V(ERR_UNAVAILABLE);
}
Error DirAccessJAndroid::remove(String p_name){

	ERR_FAIL_V(ERR_UNAVAILABLE);
}

//FileType get_file_type() const;
size_t DirAccessJAndroid::get_space_left() {

	return 0;
}


void DirAccessJAndroid::setup( jobject p_io) {


	JNIEnv *env = ThreadAndroid::get_env();
	io=p_io;
	__android_log_print(ANDROID_LOG_INFO,"godot","STEP7");

	jclass c = env->GetObjectClass(io);
	cls = (jclass)env->NewGlobalRef(c);
	__android_log_print(ANDROID_LOG_INFO,"godot","STEP8");

	_dir_open = env->GetMethodID(cls, "dir_open", "(Ljava/lang/String;)I");
	if(_dir_open != 0) {
		__android_log_print(ANDROID_LOG_INFO,"godot","*******GOT METHOD _dir_open ok!!");
	}
	_dir_next = env->GetMethodID(cls, "dir_next", "(I)Ljava/lang/String;");
	if(_dir_next != 0) {
		__android_log_print(ANDROID_LOG_INFO,"godot","*******GOT METHOD _dir_next ok!!");
	}
	_dir_close = env->GetMethodID(cls, "dir_close", "(I)V");
	if(_dir_close != 0) {
		__android_log_print(ANDROID_LOG_INFO,"godot","*******GOT METHOD _dir_close ok!!");
	}

//	(*env)->CallVoidMethod(env,obj,aMethodID, myvar);
}


DirAccessJAndroid::DirAccessJAndroid() {

	id=0;
}

DirAccessJAndroid::~DirAccessJAndroid() {

	list_dir_end();;
}
#endif