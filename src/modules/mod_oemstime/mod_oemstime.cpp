/*
* Copyright (c) 2016, California Institute of Technology.
* All rights reserved.  Based on Government Sponsored Research under contracts NAS7-1407 and/or NAS7-03001.
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
*   3. Neither the name of the California Institute of Technology (Caltech), its operating division the Jet Propulsion Laboratory (JPL),
*      the National Aeronautics and Space Administration (NASA), nor the names of its contributors may be used to
*      endorse or promote products derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE CALIFORNIA INSTITUTE OF TECHNOLOGY BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

/*
 * mod_oemstime.cpp: OnEarth module for leveraging time snapping from Mapserver requests
 * Version 1.0
 */

#include "mod_oemstime.h"

const char *twmssserviceurl;

static const char *twmssserviceurl_set(cmd_parms *cmd, void *cfg, const char *arg) {
	twmssserviceurl = arg;
	return 0;
}

static int oemstime_output_filter (ap_filter_t *f, apr_bucket_brigade *bb) {
    conn_rec *c = f->c;
    request_rec *r = f->r;

    char *srs = (char *) apr_table_get(r->notes, "oems_srs");
    char *format = (char *) apr_table_get(r->notes, "oems_format");
    char *time = (char *) apr_table_get(r->notes, "oems_time");
    char *current_layer = (char *) apr_table_get(r->notes, "oems_clayer");

	if ((ap_strstr(r->content_type, "text/xml") != 0) || (ap_strstr(r->content_type, "application/vnd.ogc.se_xml") != 0)) { // run only if Mapserver has an error due to invalid time or format
		int max_size = strlen(twmssserviceurl)+strlen(r->args);
		char *pos = 0;
		char *split;
		char *last;
	    char *new_uri = (char*)apr_pcalloc(r->pool, max_size);
	    apr_cpystrn(new_uri, twmssserviceurl, strlen(twmssserviceurl)+1);
	    split = apr_strtok(new_uri,"{SRS}",&last);
	    while(split != NULL)
	    {
	        pos = split;
	        split = apr_strtok(NULL,"{SRS}",&last);
	    }
	    srs = ap_strstr(srs, ":");
	    srs += 1;

		new_uri = apr_psprintf(r->pool,"%s%s%s?request=GetMap&layers=%s&srs=EPSG:%s&format=%s&styles=&time=%s&width=512&height=512&bbox=-1,1,-1,1",new_uri, srs, pos, current_layer, srs, format, time);
		ap_log_cerror(APLOG_MARK, APLOG_WARNING, 0, c, "oemstime bounce: %s", new_uri);
		ap_internal_redirect(new_uri, r); // redirect for handling of time by mod_onearth
	}

    return ap_pass_brigade(f->next, bb) ;
}

// Configuration options that go in the httpd.conf
static const command_rec cmds[] =
{
	AP_INIT_TAKE1(
		"TWMSServiceURL",
		(cmd_func) twmssserviceurl_set,
		0, /* argument to include in call */
		ACCESS_CONF, /* where available */
		"URL of TWMS endpoint" /* help string */
	),
	{NULL}
};

static void register_hooks(apr_pool_t *p) {
	ap_register_output_filter("OEMSTIME_OUT", oemstime_output_filter, NULL, AP_FTYPE_RESOURCE) ;
}

module AP_MODULE_DECLARE_DATA oemstime_module = {
    STANDARD20_MODULE_STUFF,
    0, 0, 0, 0, cmds, register_hooks
};