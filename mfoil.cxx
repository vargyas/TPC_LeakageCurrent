#include "mfoil.h"

#ifndef MFOIL_CXX
#define MFOIL_CXX

//----------------------------------------------------------------------------------
MFoil::MFoil() :
    fInFileName(0),
    fName(0),
    fType(0),
    fNumChannels(24),
    fLimit(0),
    fInTree(0),
    fFlagIsProcessed(kFALSE),
    fFlagIsLoaded(kFALSE),
    fFlagQuality(0),
    fQualityText(0),
    fMeasurementStart(0),
    fMeasurementEnd(0),
    fhCurrentTime(0),
    fhCurrentStd(0),
    fSatCurrent(0),
    fhLimitTime(0),
    fhLimitStd(0),
    fhCurrentCorr(0)
{
    // default constructor
    fhCurrentTime.SetOwner(kTRUE);
    fhCurrentStd.SetOwner(kTRUE);
    fhCurrentCorr.SetOwner(kTRUE);
}
//----------------------------------------------------------------------------------
MFoil::~MFoil() 
{
    // destructor
    // delete fInTree;
}
//----------------------------------------------------------------------------------
void MFoil::SetFileName(const TString filename)
{
    fInFileName = filename;

    // determine type (and number of channels):
    if (fInFileName.Contains("I_")) {
        fType = 0;
        fNumChannels = 18;
    }
    else if (fInFileName.Contains("O1_")) {
        fType = 1;
        fNumChannels = 20;
    }
    else if (fInFileName.Contains("O2_")) {
        fType = 2;
        fNumChannels = 22;
    }
    else if (fInFileName.Contains("O3_")) {
        fType = 3;
        fNumChannels = 24;
    }
    else {
        std::cerr << "Foil type not recognized!\n";
        fType = 4;
        fNumChannels = 18;
    }

    // determine subtype: protection resistors differ for different foils,
    // setting the accepted limit accordingly
    if (fInFileName.Contains("_G1_") || fInFileName.Contains("_G2_") || fInFileName.Contains("_G3_"))
        fLimit = 0.160; // nA
    else if (fInFileName.Contains("_G4_"))
        fLimit = 0.046; // nA
    else fLimit = 0.5; // default in nA
}
//----------------------------------------------------------------------------------
void MFoil::LoadFoilCurrents(const TString filename)
{   
    std::cout << "LoadFoilCurrents()\n";

    // setting the file name and determine the foil type from that
    SetFileName(filename);

    // not yet processed, setting flag
    fFlagIsProcessed = kFALSE;
    
    // clear histogram array before loading, if loaded already
    if (fFlagIsLoaded) {
        std::cout << "clearing histograms...\n";
        fhCurrentStd.Clear();
        fhCurrentTime.Clear();
        fhCurrentCorr.Clear();
        delete fInTree;
    }

    // read input file into a tree
    fInTree = new TTree("leakage_tree","Leakage current measurement");
    fInTree->ReadFile(fInFileName);
    fInTree->Print();

    const Int_t ndata = fInTree->GetEntries();

    //CreateHLimitTime();

	//std::vector<Double_t*> currents;

	fInTree->Draw(Form("time:I_%.2d",0),"","goff");
	Double_t * times    = fInTree->GetVal(0);
	Double_t * current0 = fInTree->GetVal(1);

    for (Int_t ich = 0; ich<fNumChannels; ich++)
    {
        // read variables (SetBranchAddress doesn't work)
		fInTree->Draw(Form("time:I_%.2d",ich),"","goff");
		//Double_t * times   = new Double_t[ndata];
		//times = fInTree->GetVal(0);
		//Double_t * current = new Double_t[ndata];
		//current = fInTree->GetVal(1);
		//Double_t* times = fInTree->GetVal(0);
		Double_t * current = fInTree->GetVal(1);

		//fInTree->SetBranchAddress("time", &times);
		//if(fInTree->GetListOfBranches()->FindObject(Form("I_%.2d",ich)) );
		//	fInTree->SetBranchAddress(Form("I_%.2d",ich), &current);
		//currents.push_back(current);

        Double_t width = (times[2]-times[1])/2.;
        fMeasurementStart = times[0]-width;
        fMeasurementEnd   = times[ndata-1]+width;

        fhCurrentStd.Add( new TH1D(Form("hCurrentStd_CH%d", ich), Form("CH %d", ich), 1000, -fLimit, fLimit) );


		fhCurrentTime.Add( new TGraph(ndata, times, current) );
		((TGraph*)fhCurrentTime.At(ich))->SetTitle("");

		fhCurrentCorr.Add( new TGraph(ndata, current0, current ) );
        ((TGraph*)fhCurrentCorr.At(ich))->SetTitle("");


        for( Int_t idata=1; idata<=ndata; idata++)
            ((TH1D*)fhCurrentStd.At(ich))->Fill(current[idata]);

        ((TH1D*)fhCurrentStd.At(ich))->SetXTitle("");
        ((TH1D*)fhCurrentStd.At(ich))->SetYTitle("");
        SetAxisStyle(((TH1D*)fhCurrentStd.At(ich)));


        //Double_t ymax = ((TH1D*)fhCurrentStd.At(ich))->GetMaximum();
        //CreateHLimitStd(ich, ymax);
    }

    fFlagIsLoaded = kTRUE;
}
//----------------------------------------------------------------------------------
void MFoil::ProcessFoilCurrents()
{   
    if(fFlagIsProcessed)
    {
        fSatCurrent.clear();
        fFlagQuality.clear();
    }
    
    // estimating saturation current
    for (Int_t ich = 0; ich < fNumChannels; ++ich)
    {
        fSatCurrent.push_back( ((TH1D*)fhCurrentStd.At(ich))->GetMean() );
    }
    // TODO: fit saturation current

    // determine foil quality
    for (Int_t ich = 0; ich < fNumChannels; ++ich)
    {
        if(fabs(fSatCurrent.at(ich))>=fLimit) fFlagQuality.push_back(0);        // bad foil
        else if(fabs(fSatCurrent.at(ich))<fLimit ) fFlagQuality.push_back(1);   // good foil
        else  fFlagQuality.push_back(2);                                        // strange foil
    }   
    fFlagIsProcessed = kTRUE;
}
//---------------------------------------------------------------------------------- 
Double_t GetMax(Double_t * array, int narray)
{
    Double_t xmax = array[0];
    for(int i=1; i<narray; i++)
    {
        if(array[i]>xmax) xmax=array[i];
    }
    return xmax;
}


//---------------------------------------------------------------------------------- 
int MFoil::GetNC() { return fNumChannels; }
//----------------------------------------------------------------------------------
int MFoil::GetType() { return fType; }
//----------------------------------------------------------------------------------
Bool_t MFoil::GetProcessedStatus() const { return fFlagIsProcessed; }
//----------------------------------------------------------------------------------
Bool_t MFoil::GetLoadedStatus() const { return fFlagIsLoaded; }
//----------------------------------------------------------------------------------
TString MFoil::GetInfoSatCurrent(Int_t foil_id) const
{
    return Form("I_{sat} = %.3f nA\n",fSatCurrent[foil_id]);
}//----------------------------------------------------------------------------------
TString MFoil::GetInfoLimitCurrent() const
{
    return Form("I_{limit} = %.3f nA\n",fLimit);
}
//----------------------------------------------------------------------------------

TString MFoil::GetInFileName() const { return fInFileName; }
//----------------------------------------------------------------------------------
TString MFoil::GetName() const { return fName; }
//----------------------------------------------------------------------------------
void MFoil::CreateHLimitTime()
{
    fhLimitTime = new TH1D("hLimitTime", "", 2000, fMeasurementStart, fMeasurementEnd);
    for(Int_t ib=1; ib<2000; ib++)
    {
        fhLimitTime->SetBinContent(ib, 0.0);
        fhLimitTime->SetBinError(ib, fLimit);
    }
    fhLimitTime->Print();
}
//----------------------------------------------------------------------------------
void MFoil::CreateHLimitStd(Int_t ich, Double_t ymax)
{
    fhLimitStd = new TH1D(Form("hLimitStd_CH%d",ich), "", 2000, -fLimit, fLimit);
    for(Int_t ib=1; ib<2000; ib++)
    {
        fhLimitStd->SetBinContent(ib, 0.0);
        fhLimitStd->SetBinError(ib, ymax); // limit is 0.5 nA
    }
}
//----------------------------------------------------------------------------------
// Gets the color code of the foil 

// from Rtypes.h { kWhite =0,   kBlack =1,   kGray=920,
//                 kRed   =632, kGreen =416, kBlue=600, kYellow=400, kMagenta=616, kCyan=432,
//                 kOrange=800, kSpring=820, kTeal=840, kAzure =860, kViolet =880, kPink=900 };
Int_t MFoil::GetProcessedColor(Int_t ich) const 
{
    // 0=bad (red), 1=good (green), 2=problematic (orange)
    Int_t color;
    
    if(!fFlagIsProcessed) color=920;
    else 
    {
        switch(fFlagQuality.at(ich))
        {
            case 0: color=632+2; break; 
            case 1: color=416+2; break; 
            case 2: color=800+2; break; 
            default: color=920; break; 
        }
    }
    return color;
}
    
//----------------------------------------------------------------------------------
void MFoil::DrawHLimitTime(Int_t ich)
{   
    fhLimitTime->SetFillStyle(3003);
    fhLimitTime->SetFillColor(GetProcessedColor(ich));
    fhLimitTime->SetLineColor(GetProcessedColor(ich));
    fhLimitTime->Draw("E3 same");
}
//----------------------------------------------------------------------------------
void MFoil::DrawHLimitStd(Int_t ich)
{
    fhLimitStd->SetFillStyle(3003);
    fhLimitStd->SetFillColor(GetProcessedColor(ich));
    fhLimitStd->SetLineColor(GetProcessedColor(ich));
    fhLimitStd->Draw("E3 same");
}

//----------------------------------------------------------------------------------
void  MFoil::DrawSatCurrentTime(Int_t ich)
{
    TLine * lcurrent = new TLine(fMeasurementStart, fSatCurrent.at(ich),fMeasurementEnd,  fSatCurrent.at(ich));
    lcurrent->SetLineColor(kBlack);
    lcurrent->SetLineWidth(3);
    lcurrent->Draw();
}
//----------------------------------------------------------------------------------
void  MFoil::DrawSatCurrentStd(Int_t ich)
{
    TLine * lcurrent = new TLine(fSatCurrent.at(ich),0, fSatCurrent.at(ich), ((TH1D*)fhCurrentStd.At(ich))->GetMaximum());
    lcurrent->SetLineColor(kBlack);
    lcurrent->SetLineWidth(1);
    lcurrent->SetLineStyle(2);
    lcurrent->Draw();
}
//----------------------------------------------------------------------------------
void  MFoil::DrawCorrCurrent()
{
    if(!GetLoadedStatus()) return;
    //Double_t xmin = ((TGraph*)fhCurrentCorr.At(0))->GetXaxis()->GetXmin();
    //Double_t xmax = ((TGraph*)fhCurrentCorr.At(0))->GetXaxis()->GetXmax();

    TLine * lcorr = new TLine(-fLimit/2., -fLimit/2., fLimit/2., fLimit/2.);
    lcorr->Draw();
}

//----------------------------------------------------------------------------------
void  MFoil::DrawMeasurementRange(Int_t ich)
{
    TLine * lrangeLow = new TLine(fMeasurementStart, fSatCurrent.at(ich)/10., fMeasurementStart, fSatCurrent.at(ich)*10.);
    TLine * lrangeHigh= new TLine(fMeasurementEnd, fSatCurrent.at(ich)/10., fMeasurementEnd, fSatCurrent.at(ich)*10.);
    lrangeLow->SetLineWidth(2);
    lrangeHigh->SetLineWidth(2);
    
    lrangeLow->Draw();
    lrangeHigh->Draw();
}
//----------------------------------------------------------------------------------
void MFoil::DrawLimitStd(Int_t ich)
{
    TLine * low  = new TLine(-fLimit, 0, -fLimit, ((TH1D*)fhCurrentStd.At(ich))->GetMaximum() );
    TLine * high = new TLine(fLimit, 0, fLimit, ((TH1D*)fhCurrentStd.At(ich))->GetMaximum() );
    low->Draw();
    high->Draw();
}

//----------------------------------------------------------------------------------

void MFoil::DrawLimitTime()
{
    TLine * low  = new TLine(fMeasurementStart, -fLimit, fMeasurementEnd, -fLimit );
    TLine * high = new TLine(fMeasurementStart, fLimit, fMeasurementEnd, fLimit );
    low->SetLineWidth(1);
    low->SetLineStyle(2);
    high->SetLineWidth(1);
    high->SetLineStyle(2);
    low->Draw();
    high->Draw();
}
//----------------------------------------------------------------------------------

void MFoil::DrawCurrentTime(Int_t ich, TPad * p, Bool_t drawaxes)
{
    gStyle->SetOptStat(0);

    TGraph * g = ((TGraph*)fhCurrentTime.At(ich));
    g->SetLineColor( GetProcessedColor(ich) );
    if(drawaxes)
    {
        SetAxisStyle(g);
        g->GetXaxis()->CenterTitle();
        g->GetYaxis()->CenterTitle();
        g->GetXaxis()->SetTitle("time [sec]");
        g->GetYaxis()->SetTitle("leakage current [nA]");
    }

    g->Draw("AL");
    g->GetYaxis()->SetRangeUser(-2.*fLimit,2.*fLimit);

    if(GetProcessedStatus())
    {
        DrawLimitTime();

        DrawSatCurrentTime(ich);

        if(drawaxes)
        {
            TLegend * tinfo = new TLegend(0.2, 0.2, 0.8, 0.3, "", "brNDC");
            tinfo->SetFillStyle(0); tinfo->SetBorderSize(0); tinfo->SetTextSize(0.05);
            tinfo->AddEntry((TObject*)0,GetInfoSatCurrent(ich), "");
            tinfo->AddEntry((TObject*)0,GetInfoLimitCurrent(), "");
            tinfo->Draw();

            TLegend * tch = new TLegend(0.2, 0.75, 0.8, 0.98, "", "brNDC");
            tch->SetFillStyle(0); tch->SetBorderSize(0); tch->SetTextSize(0.1);
            tch->AddEntry((TObject*)0,Form("CH %d",ich), "");
            tch->Draw();
        }
        else
        {
            TLegend * tch = new TLegend(0.2, 0.75, 0.8, 0.98, "", "brNDC");
            tch->SetFillStyle(0); tch->SetBorderSize(0); tch->SetTextSize(0.15);
            tch->AddEntry((TObject*)0,Form("CH %d",ich), "");
            tch->Draw();
        }
    }

    p->Modified();
    p->Update();
}

//----------------------------------------------------------------------------------
void MFoil::DrawCurrentStd(Int_t ich, TPad * p, Bool_t drawaxes)
{
    gStyle->SetOptStat(0);
    gStyle->SetOptTitle(0);

    TH1D * h = ((TH1D*)fhCurrentStd.At(ich));
    h->SetLineColor( GetProcessedColor(ich) );
    h->SetMarkerColor( GetProcessedColor(ich) );

    if(drawaxes)
    {
        SetAxisStyle(h);
        h->GetXaxis()->CenterTitle();
        h->GetYaxis()->CenterTitle();
        h->GetXaxis()->SetTitle("leakage current [nA]");
        h->GetYaxis()->SetTitle("occurence");
    }

    h->Draw("HIST");

    if(GetProcessedStatus())
    {
        DrawLimitStd(ich);

        DrawSatCurrentStd(ich);

        if(drawaxes)
        {
            TLegend * tch = new TLegend(0.2, 0.7, 0.5, 0.8);
            //tch->SetFillStyle(3001); tch->SetFillColor(kWhite);
            tch->SetBorderSize(0); tch->SetTextSize(0.1);
            tch->AddEntry((TObject*)0,Form("CH %d",ich), "");
            tch->Draw();

            TLegend * tinfo = new TLegend(0.2, 0.3, 0.5, 0.4);
            //tinfo->SetFillStyle(3001); tinfo->SetFillColor(kWhite);
            tinfo->SetBorderSize(0); tinfo->SetTextSize(0.05);
            tinfo->AddEntry((TObject*)0,GetInfoSatCurrent(ich), "");
            tinfo->AddEntry((TObject*)0,GetInfoLimitCurrent(), "");
            tinfo->Draw();
        }
        else
        {
            TLegend * tch = new TLegend(0.1, 0.7, 0.2, 0.8);
            //tch->SetFillStyle(3001); tch->SetFillColor(kWhite);
            tch->SetBorderSize(0); tch->SetTextSize(0.15);
            tch->AddEntry((TObject*)0,Form("CH %d",ich), "");
            tch->Draw();
        }
    }

    p->Modified();
    p->Update();
}
//----------------------------------------------------------------------------------
void MFoil::DrawCurrentCorr(Int_t ich, TPad * p, Bool_t drawaxes)
{
    gStyle->SetOptStat(0);

    TGraph * g = ((TGraph*)fhCurrentCorr.At(ich));
    g->SetLineColor( GetProcessedColor(ich) );

    if(drawaxes)
    {
        SetAxisStyle(g);
        g->GetXaxis()->CenterTitle();
        g->GetYaxis()->CenterTitle();
        g->GetXaxis()->SetTitle("I_{0} [nA]");
        g->GetYaxis()->SetTitle(Form("I_{%d} [nA]",ich));
        g->SetMarkerStyle(24);
        g->SetMarkerSize(0.1);
    }

    g->Draw("AP");
    g->GetXaxis()->SetRangeUser(-fLimit/2.,fLimit/2.);
    g->GetYaxis()->SetRangeUser(-fLimit/2.,fLimit/2.);

    if(GetProcessedStatus())
    {
        DrawCorrCurrent();

        if(drawaxes)
        {
            //TLegend * tinfo = new TLegend(0.2, 0.2, 0.8, 0.3, "", "brNDC");
            //tinfo->SetFillStyle(0); tinfo->SetBorderSize(0); tinfo->SetTextSize(0.05);
            //tinfo->AddEntry((TObject*)0,GetInfoSatCurrent(ich), "");
            //tinfo->AddEntry((TObject*)0,GetInfoLimitCurrent(), "");
            //tinfo->Draw();

            TLegend * tch = new TLegend(0.2, 0.75, 0.8, 0.98, "", "brNDC");
            tch->SetFillStyle(0); tch->SetBorderSize(0); tch->SetTextSize(0.1);
            tch->AddEntry((TObject*)0,Form("CH %d",ich), "");
            tch->Draw();
        }
        else
        {
            TLegend * tch = new TLegend(0.2, 0.75, 0.8, 0.98, "", "brNDC");
            tch->SetFillStyle(0); tch->SetBorderSize(0); tch->SetTextSize(0.15);
            tch->AddEntry((TObject*)0,Form("CH %d",ich), "");
            tch->Draw();
        }
    }

    p->Modified();
    p->Update();
}

//----------------------------------------------------------------------------------
void MFoil::SetPadMargins(TCanvas * c)
{
    TVirtualPad * p = c->GetPad(0);
    p->SetLeftMargin(0.15);
    p->SetRightMargin(0.06);
    p->SetTopMargin(0.06);
    p->SetBottomMargin(0.15);
    p->Update();
}
//----------------------------------------------------------------------------------
void MFoil::SetAxisStyle(TGraph * h)
{
    TAxis * ax = h->GetXaxis();
    TAxis * ay = h->GetYaxis();
    
    ax->SetTitleOffset(1.1);
    ax->SetLabelOffset(0.01);
    ay->SetTitleOffset(1.1);
    ay->SetLabelOffset(0.01);
    ax->SetTitleSize(0.06);
    ay->SetTitleSize(0.06);
}
//----------------------------------------------------------------------------------
void MFoil::SetAxisStyle(TH1D * h)
{
    TAxis * ax = h->GetXaxis();
    TAxis * ay = h->GetYaxis();

    ax->SetTitleOffset(1.1);
    ax->SetLabelOffset(0.01);
    ay->SetTitleOffset(1.1);
    ay->SetLabelOffset(0.01);
    ax->SetTitleSize(0.06);
    ay->SetTitleSize(0.06);
}



#endif
