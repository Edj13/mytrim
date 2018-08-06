#ifndef TRIMVACCOUNT_H
#define TRIMVACCOUNT_H

#include "ThreadedTrimBase.h"

using namespace MyTRIM_NS;

class TrimVacCount : public ThreadedTrimBase
{
public:
  TrimVacCount(SimconfType * simconf, SampleBase * sample);

protected:
  virtual void vacancyCreation();
  virtual void replacementCollision();

  virtual void threadJoin(const ThreadedTrimBase & ttb);
  virtual void writeOutput();

private:
  /// histogram of vacancies and replacements  created per unit depth (Ang)
  std::vector<unsigned int> _vac_bin;
  std::vector<unsigned int> _repl_bin;
};

#endif // TRIMVACCOUNT_H
