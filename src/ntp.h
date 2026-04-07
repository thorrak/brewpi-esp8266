#pragma once

#include <stddef.h>

/**
 * \brief Initialize and synchronize NTP time
 */
void initNTP();

/**
 * \brief Check if NTP time has been synchronized
 * \return true if time is synchronized
 */
bool isNtpSynced();

/**
 * \brief Get current time as formatted string (UTC)
 * \param buffer - Buffer to store the formatted time
 * \param bufferSize - Size of the buffer
 * \return true if time was successfully formatted
 */
bool getFormattedTime(char* buffer, size_t bufferSize);
