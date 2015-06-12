/** 
*   @file       integration.c

*   @brief      Functions to call GSL library options for integrating ODEs.

*   @bug        Event check integration may needs more tests, a function to rotate trajectories is
                also present under perron.c

*   @copyright  MERL[2013]
**/
#include "integration.h"

int evolve_pt(double curr_t, double tau, double curr_pos[], double *iter_pos, double params)

{
    //Setting up the GSL integrator routines
    const gsl_odeiv_step_type *T = gsl_odeiv_step_rkf45;
    gsl_odeiv_step *s = gsl_odeiv_step_alloc(T, NDIM);
    
    gsl_odeiv_control *c = gsl_odeiv_control_y_new(1e-12, 1e-12);
    gsl_odeiv_evolve *e = gsl_odeiv_evolve_alloc(NDIM);

    gsl_odeiv_system sys = {velocity_field, NULL, NDIM, &params};

    // gsl_odeiv_system sys = {double_gyre, NULL, NDIM, &params};

    int i;
    double t, tNext;
    double tMin, tMax, deltaT;
    double h = 1e-6; 
    double y[NDIM];
    
    tMin = curr_t;
    tMax = curr_t + tau;
    deltaT = tau/1;

    //Initial conditions for integrator
    for (i = 0; i < NDIM; i++)
    {
        y[i] = curr_pos[i];        
    }
    
    t = tMin;    //Initialization of time variable
    for (tNext = tMin + deltaT; tNext <= tMax; tNext += deltaT)
    {
        while (t < tNext)
        {
            int status = gsl_odeiv_evolve_apply(e, c, s, &sys, &t, tNext, &h, y);
        
            if (status != GSL_SUCCESS)
            {
                printf ("error, return value=%d\n", status);
                break;
            }
        }
        // printf("%lf %lf %lf\n", t, y[0],y[1]);
    }

    for (i = 0; i < NDIM; i++)
    {
        iter_pos[i] = y[i]; 
    }

    // printf("%lf %lf %lf %lf\n",curr_pos[0], curr_pos[1], iter_pos[0], iter_pos[1]);
    gsl_odeiv_evolve_free(e);
    gsl_odeiv_control_free(c);
    gsl_odeiv_step_free(s);
    
    return EXIT_SUCCESS;
}

int evolve_pt_time_event(double curr_t, double tau, double curr_pos[], double *iter_pos)
{

    double loc_in[NDIM];
    double loc_out[NDIM];
    double posInSecondCell[NDIM];
    int intContinues = 1;
    double eventTime;
    int eventFlag;
    int numHalfCells = 0;
    int i;
    double chi = PARAMETER_03;		//Angle set here!!!
    double timeSpan[2], timeSpanNew[2];

    timeSpan[0] = curr_t;
    timeSpan[1] = tau;

    loc_in[0] = curr_pos[0];
    loc_in[1] = curr_pos[1];
    
    while (intContinues == 1)
    {
        loc_in[2] = 0;
        iterate_event_check(timeSpan, loc_in, loc_out, &eventTime, &eventFlag);
        if ( eventFlag == 0)
            intContinues = 0;
        else
        {
            numHalfCells += 1;
            rotate_pos(loc_out, posInSecondCell, chi);
            timeSpanNew[0] = eventTime;
            timeSpanNew[1] = timeSpan[1];
            posInSecondCell[2] = 0;
            iterate_event_check(timeSpanNew, posInSecondCell, loc_out, &eventTime, &eventFlag); 
            if ( eventFlag == 0)
            {
                rotate_pos(loc_out, posInSecondCell, chi);
                intContinues = 0;   
                for (i=0;i<NDIM;i++)
                    loc_out[i] = posInSecondCell[i];
            }
            else
            {
                numHalfCells += 1;
                rotate_pos(loc_out, posInSecondCell, chi);
                timeSpan[0] = eventTime;
                timeSpan[1] = timeSpan[1];
                for (i=0;i<NDIM;i++)
                    loc_in[i] = posInSecondCell[i];
                intContinues = 1;
            }
        }   
    }

    iter_pos[0] = loc_out[0];
    iter_pos[1] = loc_out[1];
    iter_pos[2] = loc_out[2];

    printf("x=%f\ny=%f\ntheta = %f\nEvent time = %f\nEvent flag = %d\n", iter_pos[0], iter_pos[1], iter_pos[2], eventTime, eventFlag);
    
    return EXIT_SUCCESS;
}

int iterate_event_check(double timeSpan[], double curr[], double *output, double *eventTime, int *eventFlag)
{
    //double mu_param=mu;
    int dirxn=200;          //+1 Because this is integration in positive time
    int dimcnt=0;
    double t = timeSpan[0], tf = timeSpan[1];
    double h = 1e-10;
    double y[NDIM];
    int ev_val = 1;     //-1 for detecting positive->negative 0-crossing, +1 for detecting negative->positive 0-crossing                    
    
    *eventTime = tf;

    double prevy[NDIM];
    int ev_status=0;
    int isfirst=0;
    double negt; double post; 
        double negY[NDIM]; 
//         double posY[N        DIM];
    double negf;double posf;
    int isdone=0;
 
    const gsl_odeiv_step_type * T = gsl_odeiv_step_rkf45;
    gsl_odeiv_step * s = gsl_odeiv_step_alloc (T, NDIM);
    gsl_odeiv_control * c = gsl_odeiv_control_y_new (1e-12, 1e-10);
    gsl_odeiv_evolve * e = gsl_odeiv_evolve_alloc (NDIM);
    gsl_odeiv_system sys = {twisted_pipe_time, NULL, NDIM, &dirxn};
       

    for (dimcnt=0;dimcnt<NDIM;dimcnt++)
        y[dimcnt]=curr[dimcnt];
       
    while ((t < tf) && (isdone<4)) 
    {
        for (dimcnt=0;dimcnt<NDIM;dimcnt++)
            prevy[dimcnt]=y[dimcnt];
           
        int status = gsl_odeiv_evolve_apply (e, c, s, &sys, &t, tf, &h, y);
         
        if (status != GSL_SUCCESS)
        {
            printf("MAJOR ERROR"); 
            break;
        }
           
        double curr_val = eventfun(y, ev_val);
        double prev_val = eventfun(prevy, ev_val);
      
        if (ev_status==0)       //Event handling not triggered
        {
            if (curr_val > 0 && prev_val <0)    //Check if a crossing occured during the most recent step
            {    
                ev_status=1;        //yes there has been crossing and event handling and locating begins
                isfirst=1;          //This will be used later in the code
            } 
            else
            {    
                ev_status=0;     //no crossing..keep on integrating
            }
        }
       
        if (ev_status==1)       //Event handling and locating begins    
        {
            if ( fabs(curr_val) < err_cross )       //This condition check if we have accurately found the crossing point
            { 
                ev_status = 0;                      //Stops event handling as the desired accuracy of crossing is achieved
                isdone = 1;
                *eventTime = t;
                *eventFlag = 1;
                // printf("Found intersection ...stopping integration, time = %f \n", *eventTime);
                fflush(stdin);
                break;
             
                //WE HAVE GOTTEN THE NEXT ITERATE..SO QUIT INTEGRATING AND FREE ALL MEMORY
            }   //Back to regular integration..END of event handling
        
            else  //Keep on iterating to get the best value of crossing time
            {
                // printf("still iterating\n");
                gsl_odeiv_evolve_reset(e);
                if (isfirst == 1)             //IF this is the first time we are trying to guess for this particular crossing
                {
                    isfirst=0;
                    for (dimcnt=0;dimcnt<NDIM;dimcnt++)
                    {
                        negY[dimcnt]=prevy[dimcnt];       //Current value is used for posf/post and previous value is used for negf/negt
//                      posY[dimcnt]=y[dimcnt];
                    }
                    negt=t-h;post=t;
                    negf=prev_val;posf=curr_val;
                }
                else                         //This is not the first iteration to guess the crossing time
                {  
                    if (curr_val>0 )
                    { 
                        post=t;
                        posf=curr_val;
                    }     //Choose a new value of t on the positive side (after crossing)
                    else if (curr_val<0)
                    {
                        negt=t;
                        negf=curr_val;       //Choose a new value of t on the negative side (before crossing)
                        for (dimcnt=0;dimcnt<NDIM;dimcnt++)
                            negY[dimcnt]= y[dimcnt];
                    }
                }

                //Using the neg and pos values decided above, get a guess for the time of crossing
                double t_guess=(post*negf-negt*posf)/(negf-posf);
                tf=t_guess; 
                t=negt;  //Integrate again using t_guess as final time and negt as starting time...
                for (dimcnt = 0; dimcnt<NDIM; dimcnt++)
                    y[dimcnt] = negY[dimcnt];
            }

        } //End of if ev_status==1 Loop 
        

    } //End of the time loop 
    if (isdone!=1)
    {
        // printf("NO event was found, time is %f\n",t);
        *eventTime = t;
        *eventFlag = 0;
        fflush(stdin);
    }
        
    //Free the associated memory with the integrator routines
    gsl_odeiv_evolve_free (e);
    gsl_odeiv_control_free (c);
    gsl_odeiv_step_free (s);
        
    for (dimcnt=0;dimcnt<NDIM;dimcnt++)
        output[dimcnt]=y[dimcnt];
        
    return GSL_SUCCESS;

}     

double eventfun(const double Y[], int which) 

/**
*   @brief      This is the event function (0-crossings to be detected), "which" provides the direction
                Use "which" to -1 for detecting positive->negative 0-crossing, +1 for detecting negative->positive 0-crossing
**/
{ 
    double theta = Y[2];
    double result;
    result = which*(theta - pi);

    return result; 
}

int rotate_pos(double loc_out[], double *posInSecondCell, double angleOfRotation)
{
    posInSecondCell[0] = loc_out[0]*cos(angleOfRotation) + loc_out[1]*sin(angleOfRotation);
    posInSecondCell[1] = -loc_out[0]*sin(angleOfRotation) + loc_out[1]*cos(angleOfRotation);
    posInSecondCell[2] = loc_out[2];
    
    return EXIT_SUCCESS;
}



 
  
     
     
