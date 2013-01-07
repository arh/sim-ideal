#include <float.h>
#include "global.h"
#include "parser.h"

// multiply 1000 to reach milisecond, orignial value to produce second has 3 more zero
#define WINDOWS_TICK 10000
#define SEC_TO_UNIX_EPOCH 11644473600LL

double  WindowsTickToUnixmiliSeconds(long long windowsTicks)
{
	return (double)((double)((double) windowsTicks / (double) WINDOWS_TICK) - SEC_TO_UNIX_EPOCH);
}

bool  getAndParseMSR(std::ifstream & inputTrace, reqAtom *newn)
{
	int bcount_temp = 0;
	long long int time = 0;
	int fetched = 0;
	unsigned long long int byteoff = 0;
	char *tempchar;
	char *r_w;
	char line[201];
	static long lineno;
	static double old_time = 0;
	
	assert( inputTrace.good() ); 

	while(!fetched) {
		

		std::string lineString;
		std::getline(inputTrace,  lineString);
		if(inputTrace.eof()) {
			return false;
		}
		strcpy(line, lineString.c_str());
		lineno++;
		// Sample MSR trace lien:
		// 	Timestamp        ,Hostname,DiskNumber,Type  ,Offset     ,Size,ResponseTime
		// 128166554283938750,wdev    ,3         ,Write ,3154152960,4096 ,   2170
		tempchar = strtok(line, ",");
		//FIXME: ARH: is this time satisfy disksim timing rules ? fixed for msr
		time =  strtoll(tempchar, NULL, 10);

		if(time) {
			if(WindowsTickToUnixmiliSeconds(time) <= DBL_MAX) {
				//FIXME: ARH: is this time satisfy disksim timing rules ?
				newn->issueTime = (double) WindowsTickToUnixmiliSeconds(time);
// 				new->time = new->time*1000; //manual scale by ARH
			} else {
				fprintf(stderr, "ARH: request time reach to the double boundry\n");
				fprintf(stderr, "line: %s", line);
				ExitNow(1);
			}

// 			if( old_time > newn->time){
// 			  fprintf(stderr, "ARH: new time is small equal than old time\n");
// 				fprintf(stderr, "line: %s", line);
// 				ddbg_assert(0);
// 			}
			if(old_time >= newn->issueTime) {
				newn->issueTime = old_time + 1;
				/*				fprintf(stderr, "ARH: new time is small equal than old time\n");
								fprintf(stderr, "line: %s", line);
								ddbg_assert(0);*/
			}

			if(old_time > newn->issueTime) {
				fprintf(stderr, "ARH: new time is small equal than old time\n");
				fprintf(stderr, "line: %s", line);
				ExitNow(1);
			}

			old_time = newn->issueTime;
			strtok(NULL, ","); //step over host name
			strtok(NULL, ","); //step over devno
			//ARH: msr traces only have one dev
			r_w = strtok(NULL, ","); //step over type

			if(strcmp(r_w, "Write") == 0) {
				newn->flags = WRITE;
			} else if(strcmp(r_w, "Read") == 0) {
				newn->flags = READ;
// 				continue; //only write acceess (Read bug dareh)
			} else
				continue;

			byteoff = strtoull((strtok(NULL, " ,")) , NULL , 10) ;   //read byteoffset (byte)

			if(!byteoff) {
				continue;
			}

			if(!byteoff % 512) {
				PRINT(fprintf(stderr, "ARH: request byte offset is not aligned to sector size\n"););
				PRINT(fprintf(stderr, "line: %s", line););
			} else {
				newn->fsblkno = (byteoff / _gConfiguration.fsblkSize) ; //convert byte2sector and align to page size
			}

			bcount_temp = atoi((strtok(NULL, " ,")));   // read size

			if(!bcount_temp) {
				continue;
			}

			if(!bcount_temp % 512) {
				PRINT(fprintf(stderr, "ARH: request byte count is not aligned to sector size\n"););
				PRINT(fprintf(stderr, "line: %s", line););
			} else {
				newn->reqSize = bcount_temp/_gConfiguration.fsblkSize;
			}

			fetched = 1;
			newn->lineNo = lineno;
		} //end if(time)
	}//end while fetched

	return true;
}


/*

static int iotrace_month_convert (char *monthstr, int year)
{
   if (strcmp(monthstr, "Jan") == 0) {
      return(0);
   } else if (strcmp(monthstr, "Feb") == 0) {
      return(31);
   } else if (strcmp(monthstr, "Mar") == 0) {
      return((year % 4) ? 59 : 60);
   } else if (strcmp(monthstr, "Apr") == 0) {
      return((year % 4) ? 90 : 91);
   } else if (strcmp(monthstr, "May") == 0) {
      return((year % 4) ? 120 : 121);
   } else if (strcmp(monthstr, "Jun") == 0) {
      return((year % 4) ? 151 : 152);
   } else if (strcmp(monthstr, "Jul") == 0) {
      return((year % 4) ? 181 : 182);
   } else if (strcmp(monthstr, "Aug") == 0) {
      return((year % 4) ? 212 : 213);
   } else if (strcmp(monthstr, "Sep") == 0) {
      return((year % 4) ? 243 : 244);
   } else if (strcmp(monthstr, "Oct") == 0) {
      return((year % 4) ? 273 : 274);
   } else if (strcmp(monthstr, "Nov") == 0) {
      return((year % 4) ? 304 : 305);
   } else if (strcmp(monthstr, "Dec") == 0) {
      return((year % 4) ? 334 : 335);
   }
   assert(0);
   return(-1);
}


static double iotrace_raw_get_hirestime (int bigtime, int smalltime)
{
   unsigned int loresticks;
   int small, turnovers;
   int smallticks;

   if (basebigtime == -1) {
      basebigtime = bigtime;
      basesmalltime = smalltime;
      basesimtime = 0.0;
   } else {
      small = (basesmalltime - smalltime) & 0xFFFF;
      loresticks = (bigtime - basebigtime) * 11932 - small;
      turnovers = (int) (((double) loresticks / (double) 65536) + (double) 0.5);
      smallticks = turnovers * 65536 + small;
      basebigtime = bigtime;
      basesmalltime = smalltime;
      basesimtime += (double) smallticks * (double) 0.000838574;
   }
   return(basesimtime);
}




static ioreq_event * iotrace_raw_get_ioreq_event (FILE *tracefile, ioreq_event *newn)
{
   int bigtime;
   short small;
   int smalltime;
   int failure = 0;
   char order, crit;
   double schedtime, donetime;
   int32_t val;

   failure |= iotrace_read_int32(tracefile, &val);
   bigtime = val;
   failure |= iotrace_read_short(tracefile, &small);
   smalltime = ((int) small) & 0xFFFF;
   newn->time = iotrace_raw_get_hirestime(bigtime, smalltime);
   failure |= iotrace_read_short(tracefile, &small);
   failure |= iotrace_read_int32(tracefile, &val);
   bigtime = val;
   smalltime = ((int) small) & 0xFFFF;
   schedtime = iotrace_raw_get_hirestime(bigtime, smalltime);
   failure |= iotrace_read_int32(tracefile, &val);
   bigtime = val;
   failure |= iotrace_read_short(tracefile, &small);
   smalltime = ((int) small) & 0xFFFF;
   donetime = iotrace_raw_get_hirestime(bigtime, smalltime);
   failure |= iotrace_read_char(tracefile, &order);
   failure |= iotrace_read_char(tracefile, &crit);
   if (crit) {
      newn->flags |= TIME_CRITICAL;
   }
   failure |= iotrace_read_int32(tracefile, &val);
   newn->bcount = val >> 9;
   failure |= iotrace_read_int32(tracefile, &val);
   newn->blkno = val;
   failure |= iotrace_read_int32(tracefile, &val);
   newn->devno = val;
   failure |= iotrace_read_int32(tracefile, &val);
   newn->flags = val & READ;
   newn->cause = 0;
   newn->buf = 0;
   newn->opid = 0;
   newn->busno = 0;
   newn->tempint1 = (int)((schedtime - newn->time) * (double) 1000);
   newn->tempint2 = (int)((donetime - schedtime) * (double) 1000);
   if (failure) {
      addtoextraq((event *) newn);
      newn = NULL;
   }
   return(newn);
}


static ioreq_event * iotrace_emcsymm_get_ioreq_event (FILE *tracefile, ioreq_event *newn)
{
   char line[201];
   char operation[15];
   unsigned int director;

   if (fgets(line, 200, tracefile) == NULL) {
      addtoextraq((event *) newn);
      return(NULL);
   }
   if (sscanf(line, "%lf %s %x %x %d %d\n", &newn->time, operation, &director, &newn->devno, &newn->blkno, &newn->bcount) != 6) {
      fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
      fprintf(stderr, "line: %s", line);
      ddbg_assert(0);
   }
   if (!strcasecmp(operation,"Read")) {
      newn->flags = READ;
   } else if (!strcasecmp(operation,"Write")) {
      newn->flags = WRITE;
   } else {
      fprintf(stderr, "Unknown operation: %s in iotrace event\n",operation);
      fprintf(stderr, "line: %s", line);
      exit(1);
   }
   newn->buf = 0;
   newn->opid = 0;
   newn->busno = 0;
   newn->cause = 0;
   return(newn);
}

static ioreq_event * iotrace_emcbackend_get_ioreq_event (FILE *tracefile, ioreq_event *newn)
{
   char line[201];
   char operation[15];
   unsigned int director;
   char bus[2];
   unsigned int disk, hyper;

   if (fgets(line, 200, tracefile) == NULL) {
      addtoextraq((event *) newn);
      return(NULL);
   }

   if (sscanf(line, "%lf %s %x %x %d %d %s %d %d\n",
              &newn->time, operation, &director, &hyper, &newn->blkno, &newn->bcount, bus, &disk, &newn->devno) != 9) {
      fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
      fprintf(stderr, "line: %s", line);
      exit(0);
   }
   if (!strcasecmp(operation,"Read")) {
      newn->flags = READ;
   } else if (!strcasecmp(operation,"Write")) {
      newn->flags = WRITE;
   } else {
      fprintf(stderr, "Unknown operation: %s in iotrace event\n",operation);
      fprintf(stderr, "line: %s", line);
      exit(0);
   }

   newn->time *= 1000.0;  // emc trace times are in seconds!!

   newn->buf = 0;
   newn->opid = 0;
   newn->busno = 0;
   newn->cause = 0;
   return(newn);
}


static ioreq_event * iotrace_ascii_get_ioreq_event (FILE *tracefile, ioreq_event *newn)
{
   char line[201];
#ifdef ARH
	 static long lineno;
#endif

   if (fgets(line, 200, tracefile) == NULL) {
      addtoextraq((event *) newn);
      return(NULL);
   }
   if (sscanf(line, "%lf %d %llu %d %x\n", &newn->time, &newn->devno, &newn->blkno, &newn->bcount, &newn->flags) != 5) {
      fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
      fprintf(stderr, "line: %s", line);
      ddbg_assert(0);
   }
#ifdef ARH
	 lineno++;
#endif
   if (newn->flags & ASYNCHRONOUS) {
      newn->flags |= (newn->flags & READ) ? TIME_LIMITED : 0;
   } else if (newn->flags & SYNCHRONOUS) {
      newn->flags |= TIME_CRITICAL;
   }

   newn->buf = 0;
   newn->opid = 0;
   newn->busno = 0;
   newn->cause = 0;
#ifdef ARH
	 newn->lineno=lineno;
#endif
   return(newn);
}

#ifdef ARH
ioreq_event * iotrace_blktrace_get_ioreq_event (FILE *tracefile, ioreq_event *newn)
{
  int var1;
  int var2;
	int var3;
	int var4;
	int var6;
	int var10;
	char tempchar[5];
	char r_w[5];
	char line[201];
	int fetched=0;

  while( !fetched) {

		if (fgets(line, 200, tracefile) == NULL) {
				addtoextraq((event *) newn);
				return(NULL);
		}

		if (sscanf(line+1, "%d,%d %d %d %lf %d %s %s %llu + %d",&var1,&var2,&var3,&var4,&newn->time, &var6,  tempchar, r_w, &newn->blkno ,&newn->bcount) == 10) {

			if (newn->blkno && newn->bcount && (strcmp(tempchar, "D") == 0))
					fetched=1;
		}
		else
			continue;

		if ( (strcmp(r_w, "W") == 0) || (strcmp(r_w, "WS") == 0)) {
			newn->flags=WRITE;
		}
		else if(strcmp(r_w, "R") == 0) {
			newn->flags=READ;
		}
		else{
			fetched=0;
			continue;
		}
	} //end while

   if (newn->flags & ASYNCHRONOUS) {
      newn->flags |= (newn->flags & READ) ? TIME_LIMITED : 0;
   } else if (newn->flags & SYNCHRONOUS) {
      newn->flags |= TIME_CRITICAL;
   }


   newn->buf = 0;
   newn->opid = 0;
   newn->busno = 0;
   newn->cause = 0;
   return(newn);
}
unsigned int arh_ch2dec( char ch){

	switch (ch) {
		case '0':
			return 0;
		case '1' :
			return 1;
		case '2' :
			return 2;
		case '3' :
			return 3;
		case '4' :
			return 4;
		case '5' :
			return 5;
		case '6' :
			return 6;
		case '7' :
			return 7;
		case '8' :
			return 8;
		case '9' :
			return 9;
		case 'a' :
			return 10;
		case 'b' :
			return 11;
		case 'c' :
			return 12;
		case 'd' :
			return 13;
		case 'e' :
			return 14;
		case 'f' :
			return 15;
		default :
			fprintf(stderr, "ARH: undefined hex value in trace file detected\n");
			fprintf(stderr, "character: %c\n", ch);
			ddbg_assert(0);
	}
}

unsigned long long int arh_convert_hex_int(char * str){
	int i;
	char  newstr[15];
	unsigned long long int intvalue, position;
	sscanf(str, "%s",newstr);
	while ( newstr[i] != NULL ){
		i++;
	}
	assert(newstr[0]=='0');
	assert(newstr[1]=='x');
	newstr[1]='0';
	i--; //align ith position
	position=1;
	for ( i ; i != -1 ; i--){
		intvalue = intvalue + (unsigned long long) arh_ch2dec(newstr[i] )*position ;
		position= position*16;
	}
	return intvalue;
}

ioreq_event * iotrace_arhsyn_get_ioreq_event (char  n_char[], ioreq_event *newn){

	int n;
	static double time;
	time++;

	if(time > 10000000)
		return 0;

	n=atoi(n_char);
	assert(n);

	newn->bcount = 8 ;
	newn->blkno = rand()%n+1; ; //convert byte2sector
	newn->flags=READ;
	newn->devno = 0; //step over devno
	newn->time =time;
	newn->lineno=time;

	if (newn->flags & ASYNCHRONOUS) {
		newn->flags |= (newn->flags & READ) ? TIME_LIMITED : 0;
	} else if (newn->flags & SYNCHRONOUS) {
		newn->flags |= TIME_CRITICAL;
	}

	newn->buf = 0;
	newn->opid = 0;
	newn->busno = 0;
	newn->cause = 0;
	newn->slotno=0;
	newn->ssd_elem_num=0;
	newn->ssd_gang_num=0;
	return(newn);

}


ioreq_event * iotrace_fiu_get_ioreq_event (FILE *tracefile, ioreq_event *newn){
	long long int time=0;
	int devno=0;
	int fetched=0;
	char *tempchar;
	char *r_w;
	char line[LINE_MAX];

	while(!fetched){

		if (fgets(line, 200, tracefile) == NULL) {
			addtoextraq((event *) newn);
			return(NULL);
		}

	// Sample FIU trace lien:
	// 	[ts in ns] [pid] [process] [lba] [size in 512 Bytes blocks] [Write or Read] [major device number] [minor device number] [MD5]
  // 85564152538721 318 kjournald 194192 8 W 1 0 b2b0d4272e2645dc1658d5908c48d964
		tempchar=strtok(line, " ");
		//FIXME: ARH: is this time satisfy disksim timing rules ?
		time =  strtoll(tempchar,NULL,10);

		if(time){

			if(time <= DBL_MAX){
						//FIXME: ARH: is this time satisfy disksim timing rules ?
				newn->time = (double) time;
			}
			else {
				fprintf(stderr, "ARH: request time reach to the double boundry\n");
				fprintf(stderr, "line: %s", line);
				ddbg_assert(0);
			}

			strtok(NULL, " "); //step over pid
			strtok(NULL, " "); //step over process
			tempchar = strtok(NULL, " ");

			if ( ! tempchar )
				continue;

			newn->blkno = strtoull( tempchar , NULL , 10 ) ; //read lba address

			if(!newn->blkno)
				continue;

			tempchar = strtok(NULL, " ");

			if ( ! tempchar )
				continue;

			newn->bcount = atoi ( tempchar ) ; //read bcount
			if(!newn->bcount)
				continue;

			r_w = strtok(NULL, " "); //step over type
			if (strcmp(r_w, "W") == 0) {
				newn->flags=WRITE;
			}
			else if(strcmp(r_w, "R") == 0) {
				newn->flags=READ;
			}
			else
				continue;


			newn->devno = atoi ( (strtok(NULL, " ")) ); //step over devno

			fetched=1;
		} //end if(time)
	}//end while fetched

	if (newn->flags & ASYNCHRONOUS) {
		newn->flags |= (newn->flags & READ) ? TIME_LIMITED : 0;
	} else if (newn->flags & SYNCHRONOUS) {
		newn->flags |= TIME_CRITICAL;
	}
	newn->buf = 0;
	newn->opid = 0;
	newn->busno = 0;
	newn->cause = 0;
	newn->ssd_elem_num=0;
	newn->ssd_gang_num=0;
	return(newn);
}


ioreq_event * iotrace_msn_get_ioreq_event (FILE *tracefile, ioreq_event *newn)
{
	int bcount_temp=0;
	long long int time=0;
	char *temp_dev;
	char *r_w;
	char line[301];
	char * type_req;
	int fetched=0;
	unsigned long long int byteoff=0;

	//FIXME: ARH: what addtoexraq section do?

	while(fetched==0){ //while until read one valid data from msn trace file

		if (fgets(line, 300, tracefile) == NULL) {
			addtoextraq((event *) newn);
			return(NULL);
		}

		type_req=strtok(line, " ,"); //step over access type

		if( strcmp(type_req, "DiskWrite")==0 || strcmp(type_req, "DiskRead")==0 ){
				time=atoi(strtok(NULL, " ,"));
				if(time){
						fetched=1;
					if (time >= LLONG_MAX || time <= 0 ){
						fprintf(stderr, "ARH: request time reach to the int boundry\n");
						fprintf(stderr, "line: %s", line);
						ddbg_assert(0);
					}
					if(time <= DBL_MAX){
						//FIXME: ARH: is this time satisfy disksim timing rules ?
						newn->time = (double) time;
					}
					else {
						fprintf(stderr, "ARH: request time reach to the double boundry\n");
						fprintf(stderr, "line: %s", line);
						ddbg_assert(0);
					}

					strtok(NULL, "),"); //step over process ID
	// 				strtok(NULL, " ,"); //step over process number
					strtok(NULL, " ,"); //step over thread ID
					strtok(NULL, " ,"); //step over IrpPtr
					byteoff =arh_convert_hex_int( (strtok(NULL, " ,")) ) ; //read byteoffset (byte)
					if( !byteoff ){
						fprintf(stderr, "ARH: request byte offset is ZERO !!! line parsing problem?\n");
						fprintf(stderr, "line: %s", line);
						ddbg_assert(0);
					}

					if( !byteoff%512){
						fprintf(stderr, "ARH: request byte offset is not aligned to sector size\n");
						fprintf(stderr, "line: %s", line);
						ddbg_assert(0);
					}
					else
						newn->blkno = byteoff / 512 ; //convert byte2sector

					bcount_temp = arh_convert_hex_int( (strtok(NULL, " ,")) ); // read size
					if( !bcount_temp ){
						fprintf(stderr, "ARH: request byte count is ZERO !!! line parsing problem?\n");
						fprintf(stderr, "line: %s", line);
						ddbg_assert(0);
					}

					if( !bcount_temp%512){
						fprintf(stderr, "ARH: request byte count is not aligned to sector size\n");
						fprintf(stderr, "line: %s", line);
						ddbg_assert(0);
					}
					else
						newn->bcount = bcount_temp / 512 ; //convert byte2sector
					strtok(NULL, " ,"); //step over ElapsedTime
					temp_dev=(strtok(NULL, " ,")); // read disk no
					newn->devno = atoi(  temp_dev );
				} //end if(time)
			}
	}

	// Read from line done

	if (strcmp(type_req, "DiskWrite") != 0 && strcmp(type_req, "DiskRead") !=0) {
		fprintf(stderr, "Wrong type of I/O request, MSN request should be DiskRead or Write type\n");
		fprintf(stderr, "line: %s", line);
		ddbg_assert(0);
	}
	else if (strcmp(type_req, "DiskWrite") == 0) {
		newn->flags=WRITE;
	}
	else {
		assert(strcmp(type_req, "DiskRead") == 0);
		newn->flags=READ;
	}


	if (newn->flags & ASYNCHRONOUS) {
		newn->flags |= (newn->flags & READ) ? TIME_LIMITED : 0;
	} else if (newn->flags & SYNCHRONOUS) {
		newn->flags |= TIME_CRITICAL;
	}


	newn->buf = 0;
	newn->opid = 0;
	newn->busno = 0;
	newn->cause = 0;
	return(newn);
}


#endif


static ioreq_event * iotrace_batch_get_ioreq_event (FILE *tracefile, ioreq_event *newn)
{
   char line[201];

   if (fgets(line, 200, tracefile) == NULL) {
      addtoextraq((event *) newn);
      return(NULL);
   }
   if (sscanf(line, "%lf %d %d %d %x %d\n", &newn->time, &newn->devno, &newn->blkno, &newn->bcount, &newn->flags, &newn->batchno) != 6) {
      fprintf(stderr, "Wrong number of arguments for I/O trace event type\n");
      fprintf(stderr, "line: %s", line);
      ddbg_assert(0);
   }
   if (newn->flags & ASYNCHRONOUS) {
      newn->flags |= (newn->flags & READ) ? TIME_LIMITED : 0;
   } else if (newn->flags & SYNCHRONOUS) {
      newn->flags |= TIME_CRITICAL;
   }
   if (newn->flags & BATCH_COMPLETE) {
     newn->batch_complete = 1;
   } else {
     newn->batch_complete = 0;
   }

   newn->buf = 0;
   newn->opid = 0;
   newn->busno = 0;
   newn->cause = 0;
   return(newn);
}


ioreq_event * iotrace_get_ioreq_event (FILE *tracefile, int traceformat, ioreq_event *temp)
{
   switch (traceformat) {

   case ASCII:
      temp = iotrace_ascii_get_ioreq_event(tracefile, temp);
      break;

   case RAW:
      temp = iotrace_raw_get_ioreq_event(tracefile, temp);
      break;

   case HPL:
      temp = iotrace_hpl_get_ioreq_event(tracefile, temp);
      break;

   case DEC:
      temp = iotrace_dec_get_ioreq_event(tracefile, temp);
      break;

   case VALIDATE:
      temp = iotrace_validate_get_ioreq_event(tracefile, temp);
      break;

   case EMCSYMM:
      temp = iotrace_emcsymm_get_ioreq_event(tracefile, temp);
      break;

   case EMCBACKEND:
      temp = iotrace_emcbackend_get_ioreq_event(tracefile, temp);
      break;

   case BATCH:
      temp = iotrace_batch_get_ioreq_event(tracefile, temp);
      break;
#ifdef ARH
	 case BLKTRACE:
			temp = iotrace_blktrace_get_ioreq_event(tracefile, temp);
			break;
  	case MSN:
	  	temp = iotrace_msn_get_ioreq_event(tracefile, temp);
		 break;
		case MSR:
			temp = iotrace_msr_get_ioreq_event(tracefile, temp);
			break;
		case FIU:
			 temp = iotrace_fiu_get_ioreq_event(tracefile, temp);
			 break;
		case ARHSYN:
			 temp = iotrace_arhsyn_get_ioreq_event(disksim->iotracefilename,temp);

#endif

   default:
      fprintf(stderr, "Unknown traceformat in iotrace_get_ioreq_event - %d\n", traceformat);
      exit(1);
   }

   return ((ioreq_event *)temp);
}


static void iotrace_hpl_srt_tracefile_start (char *tracedate)
{
   char crap[40];
   char monthstr[40];
   int day;
   int hour;
   int minute;
   int second;
   int year;

   if (sscanf(tracedate, "%s\t= \"%s %s %d %d:%d:%d %d\";\n", crap, crap, monthstr, &day, &hour, &minute, &second, &year) != 8) {
      fprintf(stderr, "Format problem with 'tracedate' line in HPL trace - %s\n", tracedate);
      exit(1);
   }
   if (baseyear == 0) {
      baseyear = year;
   }
   day = day + iotrace_month_convert(monthstr, year);
   if (year != baseyear) {
      day += (baseyear % 4) ? 365 : 366;
   }
   if (baseday == 0) {
      baseday = day;
   }
   second = second + (60 * minute) + (3600 * hour) + (86400 * (day - baseday));
   if (basesecond == 0) {
      basesecond = second;
   }
   second -= basesecond;
   tracebasetime += (double) 1000 * (double) second;
}


static void iotrace_hpl_initialize_file (FILE *tracefile, int print_tracefile_header)
{
   char letter = '0';
   char line[201];
   char linetype[40];

   if (disksim->traceheader == FALSE) {
      return;
   }
   while (1) {
      if (fgets(line, 200, tracefile) == NULL) {
         fprintf(stderr, "No 'tracedate' line in HPL trace\n");
         exit(1);
      }
      sscanf(line, "%s", linetype);
      if (strcmp(linetype, "tracedate") == 0) {
         break;
      }
   }
   iotrace_hpl_srt_tracefile_start(line);
   while (letter != 0x0C) {
      if (fscanf(tracefile, "%c", &letter) != 1) {
         fprintf(stderr, "End of header information never found - end of file\n");
         exit(1);
      }
      if ((print_tracefile_header) && (letter != 0x0C)) {
         printf("%c", letter);
      }
   }
}


void iotrace_initialize_file (FILE *tracefile, int traceformat, int print_tracefile_header)
{
   if (traceformat == HPL) {
      iotrace_hpl_initialize_file(tracefile, print_tracefile_header);
   }
}


void iotrace_printstats (FILE *outfile)
{
   if (disksim->iotrace_info == NULL) {
      return;
   }
   if (hpreads | hpwrites) {
      fprintf (outfile, "\n");
      fprintf(outfile, "Total reads:    \t%d\t%5.2f\n", hpreads, ((double) hpreads / (double) (hpreads + hpwrites)));
      fprintf(outfile, "Total writes:   \t%d\t%5.2f\n", hpwrites, ((double) hpwrites / (double) (hpreads + hpwrites)));
      fprintf(outfile, "Sync Reads:  \t%d\t%5.2f\t%5.2f\n", syncreads, ((double) syncreads / (double) (hpreads + hpwrites)), ((double) syncreads / (double) hpreads));
      fprintf(outfile, "Sync Writes: \t%d\t%5.2f\t%5.2f\n", syncwrites, ((double) syncwrites / (double) (hpreads + hpwrites)), ((double) syncwrites / (double) hpwrites));
      fprintf(outfile, "Async Reads: \t%d\t%5.2f\t%5.2f\n", asyncreads, ((double) asyncreads / (double) (hpreads + hpwrites)), ((double) asyncreads / (double) hpreads));
      fprintf(outfile, "Async Writes:\t%d\t%5.2f\t%5.2f\n", asyncwrites, ((double) asyncwrites / (double) (hpreads + hpwrites)), ((double) asyncwrites / (double) hpwrites));
   }
}
*/

