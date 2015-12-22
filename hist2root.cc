//###############################################
// Developed by Ivan Pogrebnyak
// MSU 2015
//###############################################

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include <TFile.h>
#include <TDirectory.h>
#include <TH1.h>

using namespace std;

void err(int l, const string& line, const string& comment) {
  cerr << "\033[31mLine "<<l<<":\033[0m " << line << endl;
  cerr << comment << endl;
  exit(1);
}

void scale(TDirectory* dir, double factor) noexcept {
  TIter next(dir->GetList());
  TObject *obj;
  while ((obj = next())) {
    if (obj->InheritsFrom(TDirectory::Class())) {
      scale(static_cast<TDirectory*>(obj),factor);
    } else if (obj->InheritsFrom(TH1D::Class())) {
      TH1D *h = static_cast<TH1D*>(obj);
      // h->Scale(factor);
      h->SetAt(0.,0);
      const Int_t n = h->GetNbinsX();
      for (Int_t i=1; i<=n; ++i)
        h->SetAt(h->GetAt(i)*h->GetBinWidth(i)/factor,i);
      h->SetAt(n+1,0);
    }
  }
}

enum line_t { Begin, Prop, Bin, End };
enum opt_t { opt_none, opt_i, opt_o };

int main(int argc, char **argv)
{
  if (argc==1) {
    cout << "Usage: " << argv[0] << " [-w] -i in.hist -o out.root" << endl;
    return 0;
  }

  bool toweights = false;
  const char *fin_name=NULL, *fout_name=NULL;

  for (int i=1; i<argc; ++i) {
    static opt_t opt = opt_none;
    switch (opt) {
      case opt_i:
        fin_name = argv[i];
        opt = opt_none;
        break;

      case opt_o:
        fout_name = argv[i];
        opt = opt_none;
        break;

      case opt_none:
        if (!strcmp(argv[i],"-w")) {
          toweights = true;
        } else if (!strcmp(argv[i],"-i")) {
          opt = opt_i;
        } else if (!strcmp(argv[i],"-o")) {
          opt = opt_o;
        }
    }
  }

  if (fout_name==NULL) {
    cout << "Arguments error: output file name not provided" << endl;
    return 1;
  }

  TFile *fout = new TFile(fout_name,"recreate");
  if (fout->IsZombie()) return 1;

  TH1::SetDefaultSumw2(false);

  string line;
  istream * const fin = (fin_name ? new ifstream(fin_name) : &cin);
  if (!fin_name || static_cast<ifstream*>(fin)->is_open()) {
    if (fin_name) cout << "\033[32mReading:\033[0m " << fin_name << endl;
    string name, title;
    vector<Double_t> bins, vals; //, errs;
    // bins.reserve(100);
    Double_t xlow, xhigh, val, errminus, errplus, events=1;
    bool first_bin = false;
    TH1 *hist;
    line_t fsm = Begin;
    while ( getline (*fin,line) ) {
      static int l = 0;
      ++l;
      if (!line.size()) continue;

      switch (fsm) {
        case Begin:
          if (!line.compare(0,17,"# BEGIN HISTOGRAM")) {
            fsm = Prop;
          } else if (toweights && !line.compare(0,12,"### Finalize")) {
            // ### Finalize: groups 50000000, events 50000000 (binned 36067018) [trials 50000000]
            const size_t n = line.find("events",13) + 7;
            events = atof(line.substr(n,line.find(' ',n)).c_str());
          } else if (line[0]=='#') continue;
          else err(l, line, "Not a comment or \"# BEGIN HISTOGRAM\"");
          break;
        case Prop:
          if (line[0]=='#') continue;
          else if (!line.compare(0,9,"AidaPath=")) {
            name  = line.substr(9);
            break;
          } else if (!line.compare(0,6,"Title=")) {
            title = line.substr(6);
            break;
          } else {
            first_bin = true;
            fsm = Bin;
          }
        case Bin:
          if (line[0]=='#') fsm = End;
          else {
            stringstream ss(line);
            ss >> xlow >> xhigh >> val >> errminus >> errplus;
            if (first_bin) {
              bins.push_back(xlow);
              first_bin = false;
            }
            bins.push_back(xhigh);
            vals.push_back(val);
            // errs.push_back(max(errminus,errplus));
            break;
          }
        case End:
          if (!line.compare(0,15,"# END HISTOGRAM")) {
            cout <<"\033[35m"<< name <<"\033[0m"<< endl;
            const size_t d1 = (name[0]=='/' ? 1 : 0);
            const size_t d2 = name.rfind('/');
            const string dir_name = name.substr(d1,d2-d1);
            TDirectory *dir = (TDirectory*)fout->Get(dir_name.c_str());
            if (!dir) dir = fout->mkdir(dir_name.c_str());
            dir->cd();
            hist = new TH1D(
              name.substr(d2+1).c_str(),title.c_str(),bins.size()-1,&bins[0]
            );
            for (size_t i=0,n=vals.size();i<n;++i) {
              hist->SetBinContent(i+1,vals[i]);
              // hist->SetBinError(i+1,errs[i]);
            }
            name = title = string();
            bins.resize(0);
            // cout << bins.capacity() << endl;
            vals.resize(0);
            // errs.resize(0);
            fsm = Begin;
          } else if (line[0]=='#') {
            fsm = Bin;
            continue;
          } else err(l, line, "Not an \"# END HISTOGRAM\"");
          break;
      }
    }

    //fout->ls();

    if (toweights) {
      scale(fout,1./events);

      fout->cd();
      (new TH1D("N","N",1,0,1))->Fill(0.5,events);
    }

    if (fin_name) {
      static_cast<ifstream*>(fin)->close();
      delete fin;
    }
  } else {
    cout << "Unable to open file " << argv[1] << endl;
    exit(1);
  }

  fout->Write();
  fout->Close();
  cout << "\033[32mWrote:\033[0m " << fout->GetName() << endl;
  delete fout;

  return 0;
}
