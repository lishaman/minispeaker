#include <stdio.h>
#include <string.h>

#include "json.h"
#include "wifi_interface.h"

int main(int argc,char *argv[])
{
	int i;
	char *wifilist;
	char *ssid,*bssid;
	int channel,signal;
	struct json_object *new_obj, *tmp, *wifi_obj;
	//struct array_list *new_array;

	wifilist = get_wifi_scanning_results();
	new_obj = json_tokener_parse(wifilist);
	//printf("new_obj.to_string()=%s\n", json_object_to_json_string(new_obj));

	wifi_obj = json_object_object_get(new_obj, "WiFiList");
	//printf("new_array.to_string()=%s\n", json_object_to_json_string(new_obj));

	for(i = 0; i < json_object_array_length(wifi_obj); i++) {
		struct json_object *obj = json_object_array_get_idx(wifi_obj, i);
		printf("\t[%d]=%s\n", i, json_object_to_json_string(obj));


		json_object_object_get_ex(obj, "SSID", &tmp);
		printf("SSID:%s\n", json_object_get_string(tmp));

		json_object_object_get_ex(obj, "Bssid", &tmp);
		printf("BSSID:%s\n", json_object_to_json_string(tmp));

		json_object_object_get_ex(obj, "Channel", &tmp);
		printf("Channel:%d\n", json_object_get_int(tmp));

		json_object_object_get_ex(obj, "Signal level", &tmp);
		printf("Signal level:%d\n", json_object_get_int(tmp));

	}

	json_object_put(new_obj);

	free(wifilist);
	return 0;
}
