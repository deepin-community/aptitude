/** \file throttle.cc */   // -*-c++-*-

// Copyright (C) 2010 Daniel Burrows
// Copyright (C) 2015-2018 Manuel A. Fernandez Montecelo
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
// Boston, MA 02110-1301, USA.

#include "throttle.h"

// Local includes:
#include <loggers.h>

// System includes:
#include <optional>

#include <generic/util/util.h>

#include <errno.h>
#include <sys/time.h>

using aptitude::Loggers;
using logging::LoggerPtr;

namespace aptitude
{
  namespace util
  {
    namespace
    {
      class throttle_impl : public throttle
      {
        std::optional<struct timeval> last_update;

        logging::LoggerPtr logger;

        // Used to ensure that we only warn once about gettimeofday()
        // failing.
        bool wrote_time_error;

        static constexpr double update_interval = 0.7;

        void write_time_error(int errnum);

      public:
        throttle_impl();

        /** \return \b true if the timer has expired. */
        bool update_required();

        /** \brief Reset the timer that controls when the display is
         *  updated.
         */
        void reset_timer();
      };

      constexpr double throttle_impl::update_interval;

      void throttle_impl::write_time_error(int errnum)
      {
        if(!wrote_time_error)
          {
            LOG_ERROR(logger,
                      "gettimeofday() failed: " <<
                      sstrerror(errnum));
            wrote_time_error = true;
          }
      }

      throttle_impl::throttle_impl()
        : logger(Loggers::getAptitudeCmdlineThrottle()),
          wrote_time_error(false)
      {
      }

      bool throttle_impl::update_required()
      {
        if(!last_update)
          return true;
        else
          {
            // Time checking code shamelessly stolen from apt, since
            // we know theirs works.
            struct timeval now;
            if(gettimeofday(&now, 0) != 0)
              {
                write_time_error(errno);
                return false;
              }
            else
              {
                const struct timeval &last_update_time = *last_update;
                double diff =
                  now.tv_sec - last_update_time.tv_sec +
                  (now.tv_usec - last_update_time.tv_usec)/1000000.0;

                bool rval = diff >= update_interval;

                return rval;
              }
          }
      }

      void throttle_impl::reset_timer()
      {
        LOG_TRACE(logger, "Resetting the update timer.");

        struct timeval now;
        if(gettimeofday(&now, 0) != 0)
          write_time_error(errno);
        else
          last_update = now;
      }
    }

    throttle::~throttle()
    {
    }

    std::shared_ptr<throttle> create_throttle()
    {
      return std::make_shared<throttle_impl>();
    }
  }
}
