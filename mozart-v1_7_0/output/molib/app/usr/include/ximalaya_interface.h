#ifndef __XIMALAYA_INTERACE_H__
#define __XIMALAYA_INTERACE_H__

#ifdef  __cplusplus
extern "C" {
#endif
	enum ximalaya_audio_format {
		/**
		 * @brief 32bit audio file
		 */
		XIMALAYA_AUDIO_FMT_32,
		/**
		 * @brief 64bit audio file
		 */
		XIMALAYA_AUDIO_FMT_64,
		/**
		 * @brief 24bit m4a audio file
		 */
		XIMALAYA_AUDIO_FMT_24_M4A,
		/**
		 * @brief 64bit m4a audio file
		 */
		XIMALAYA_AUDIO_FMT_64_M4A,
		/**
		 * @brief invalid audio file
		 */
		XIMALAYA_AUDIO_FMT_INVALID,
	};

	/**
	 * @brief search sound by keyword.
	 *
	 * @param keyword [in] search keyword
	 * @param count [in] how many results do you want
	 *
	 * @return sound resource with json format on success(Should free it after used done), NULL on fail.
	 */
	extern char *mozart_ximalaya_search_music(char *keyword, int count);

	/**
	 * @brief parse result string
	 *
	 * @param result [in] search result
	 * @param format [in] audio format
	 * @param list [out] url list
	 * @param count [in] how many results do you want
	 *
	 * @return how many url parse the result
	 */
	extern int mozart_ximalaya_parse_result(char *result, enum ximalaya_audio_format format,
						char *list[], int count);
#ifdef  __cplusplus
}
#endif

#endif //__XIMALAYA_INTERACE_H__
