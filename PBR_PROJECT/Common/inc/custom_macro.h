#ifndef __MACRO_H
#define __MACRO_H

#define NoConstructor(class_name) \
	class_name()  = delete;		  \
	~class_name() = delete;

#endif