#pragma once

#include <exception>

struct RedisResponseException : public std::exception {
   const char * what () const throw () {
      return "Invalid response from redis server";
   }
};
