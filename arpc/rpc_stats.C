
#include <inttypes.h>
#include "arpc.h"
#include "rpc_stats.h"

namespace rpc_stats {

  /** returns diff in microseconds */
  static int64_t 
  timespec_diff (struct timespec a, struct timespec b)
  {
    return  (a.tv_nsec - b.tv_nsec) / 1000 -
      int64_t(b.tv_sec - a.tv_sec) * 1000*1000;
  }
  
  void rpc_stats_t::init (u_int64_t first_time) 
  {
    count = 1;
    time_sum = first_time;
    time_squared_sum = first_time*first_time;;
    min_time = first_time;
    max_time = first_time;
  }
  
  rpc_stat_collector_t::rpc_stat_collector_t() 
    : m_active(false) // start it off inactive
    , m_interval(10*60) // default to once every 10 mins
  {
    clock_gettime(CLOCK_REALTIME, &m_last_print);
  }
  
  void rpc_stat_collector_t::print_info() 
  {
    str line = get_stat_line();
    warn ( str(line << "\n") );
    reset();
  }
  
  static void stat2str(strbuf *out, const rpc_proc_t &proc, rpc_stats_t *stats)
  {
    *out << " | " 
	 << proc.prog << " "
	 << proc.vers << " "
	 << proc.proc << " " 
	 << stats->count << " " 
	 << stats->time_sum << " " 
	 << stats->time_squared_sum << " "
	 << stats->min_time << " " 
	 << stats->max_time;
  }
  
  str rpc_stat_collector_t::get_stat_line() 
  {
    // first we print the duration in seconds
    int64_t duration = timespec_diff(sfs_get_tsnow(), m_last_print);

    // print the epoch duration in milliseconds
    duration /= 1000;

    strbuf b;
    b << "RPC-STATS " << time(NULL) << " " << duration;
    // now we add each entry in the m_stats member
    m_stats.traverse(wrap(stat2str, &b));
    return b;
  }
  
  void rpc_stat_collector_t::reset() 
  {
    m_stats.clear();
    m_last_print = sfs_get_tsnow();
  }

  void rpc_stat_collector_t::end_call(svccb *call_obj, const timespec &strt)
  {
    if (!m_active || call_obj == NULL) {
      return;
    }

    // compute time delta here
    u_int64_t time_delta = timespec_diff(sfs_get_tsnow(), strt);
    // convert from millionths to 10 thousandths
    time_delta /= 100;
    
    //warn ("end_call: %"PRIu64"\n", time_delta);
    
    // update rpc stats
    rpc_proc_t proc_info;
    proc_info.prog = call_obj->prog();
    proc_info.vers = call_obj->vers();
    proc_info.proc = call_obj->proc();
    rpc_stats_t *stat_entry = m_stats[proc_info];
    if (stat_entry == NULL) {
      rpc_stats_t new_entry;
      new_entry.init(time_delta);
      m_stats.insert(proc_info, new_entry);
    } else {
      stat_entry->count++;
      stat_entry->time_sum += time_delta;
      stat_entry->time_squared_sum += time_delta*time_delta;
      if (stat_entry->min_time > time_delta) {
	stat_entry->min_time = time_delta;
      }
      if (stat_entry->max_time < time_delta) {
	stat_entry->max_time = time_delta;
      }
    }
    
    // if enough time has passed since the last print, print again
    if (timespec_diff(sfs_get_tsnow(), m_last_print) > 
	int64_t(m_interval) * 1000000) {
      print_info();
    }
  }
}

rpc_stats::rpc_stat_collector_t& get_rpc_stats ()
{
  static rpc_stats::rpc_stat_collector_t collector;
  return collector;
}
