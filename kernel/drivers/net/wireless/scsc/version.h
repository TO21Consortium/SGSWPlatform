/******************************************************************************
 *
 * Copyright (c) 2014 - 2016 Samsung Electronics Co., Ltd. All rights reserved
 *
 *****************************************************************************/
#ifndef __SLSI_VERSION_H__
#define __SLSI_VERSION_H__

/* TODO: temporary version.h that should be removed when auto-generating the
 * version.h from version.h.template works again.
 */

/* #define SLSI_VERSION_MAJOR          <VERSION_MAJOR> */
#define SLSI_VERSION_MAJOR_STR     "<VERSION_MAJOR>"
/* #define SLSI_VERSION_MINOR          <VERSION_MINOR> */
#define SLSI_VERSION_MINOR_STR     "<VERSION_MINOR>"
/* #define SLSI_VERSION_FIXLEVEL       <VERSION_FIXLEVEL> */
#define SLSI_VERSION_FIXLEVEL_STR  "<VERSION_FIXLEVEL>"
/* #define SLSI_VERSION_BUILD          <VERSION_BUILD> */
#define SLSI_VERSION_BUILD_STR     "<VERSION_BUILD>"
#define SLSI_RELEASE_TYPE_ENG
#define SLSI_RELEASE_TYPE_ENG_LOCAL

#define SLSI_VERSION_RELEASE_COMMIT_ID          "<CHANGE>"
#define SLSI_VERSION_RELEASE_BRANCH             "<BRANCH>"
#define SLSI_VERSION_RELEASE_USER               "<USER>"
#define SLSI_VERSION_RELEASE_HOST               "<HOST>"
#define SLSI_VERSION_RELEASE_DATE               "<DATE>"
#define SLSI_VERSION_RELEASE_TIME               "<TIME>"

#ifdef SLSI_RELEASE_TYPE_ENG
#ifdef SLSI_RELEASE_TYPE_ENG_LOCAL
#define SLSI_VERSION_STRING \
	SLSI_VERSION_MAJOR_STR "." \
	SLSI_VERSION_MINOR_STR "." \
	SLSI_VERSION_FIXLEVEL_STR "." \
	SLSI_VERSION_BUILD_STR "-" \
	SLSI_VERSION_RELEASE_USER "-" \
	SLSI_VERSION_RELEASE_HOST "-" \
	SLSI_VERSION_RELEASE_COMMIT_ID "-" \
	SLSI_VERSION_RELEASE_DATE "-" \
	SLSI_VERSION_RELEASE_TIME
#else /* SLSI_RELEASE_TYPE_ENG_LOCAL */
#define SLSI_VERSION_STRING \
	SLSI_VERSION_MAJOR_STR "." \
	SLSI_VERSION_MINOR_STR "." \
	SLSI_VERSION_FIXLEVEL_STR ".er" \
	SLSI_VERSION_BUILD_STR
#endif /* SLSI_RELEASE_TYPE_ENG_LOCAL */
#else /* SLSI_RELEASE_TYPE_ENG */
#define SLSI_VERSION_STRING \
	SLSI_VERSION_MAJOR_STR "." \
	SLSI_VERSION_MINOR_STR "." \
	SLSI_VERSION_FIXLEVEL_STR
#endif /* SLSI_RELEASE_TYPE_ENG */

/*#define SLSI_BUILD_STRING "date:" __DATE__ ","  "time:" __TIME__*/
#define SLSI_BUILD_STRING " "

#define SLSI_RELEASE_STRING \
	"changelist:"   SLSI_VERSION_RELEASE_COMMIT_ID "," \
	"branch:"       SLSI_VERSION_RELEASE_BRANCH "," \
	"user:"         SLSI_VERSION_RELEASE_USER "," \
	"host:"         SLSI_VERSION_RELEASE_HOST "," \
	"date:"         SLSI_VERSION_RELEASE_DATE "," \
	"time:"         SLSI_VERSION_RELEASE_TIME

#endif /* __SLSI_VERSION_H__ */
